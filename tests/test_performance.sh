#!/bin/bash
# TC-PERF-01 (SSH-NF-001): the prompt comes back within 1 second.
# The shell is started, then 40 light commands are sent one at a time. For each one we
# measure the time until the next prompt appears. At least 90% must take 1 second or less,
# and the first prompt must appear within 1 second of starting the shell.
# Run from the repo root:  bash tests/test_performance.sh

set -u

# Build the shell with warnings enabled.
gcc -Wall -Wextra src/shell.c -o simple_shell || { echo "FAIL: the shell does not compile"; exit 1; }
SHELL_BIN=${SHELL_BIN:-./simple_shell}

LIMIT_MS=1000
DIR=$(mktemp -d)
mkfifo "$DIR/in"
PID=""
cleanup() { exec 3>&- 2>/dev/null; [ -n "$PID" ] && kill "$PID" 2>/dev/null; rm -rf "$DIR"; }
trap cleanup EXIT

now_ms() { echo $(( $(date +%s%N) / 1000000 )); }
prompts() { grep -o "myshell> " "$DIR/out" 2>/dev/null | wc -l; }

# wait_for_prompts <count>: waits (at most 5 s) until at least <count> prompts were printed
wait_for_prompts() {
  local end=$(( $(now_ms) + 5000 ))
  while [ "$(prompts)" -lt "$1" ]; do
    [ "$(now_ms)" -ge "$end" ] && return 1
    sleep 0.005
  done
  return 0
}

# --- start-up ---
START=$(now_ms)
"$SHELL_BIN" < "$DIR/in" > "$DIR/out" 2>&1 &
PID=$!
exec 3> "$DIR/in"
wait_for_prompts 1
STARTUP_MS=$(( $(now_ms) - START ))

# --- 40 light commands ---
LIGHT=("pwd" "echo hello" "ls" "cd ." "help" "history" "nosuchcmd" "echo a | cat" "echo b > $DIR/o.txt" "cat < $DIR/o.txt")
TOTAL=0
OK=0
SLOWEST=0
SUM=0
for round in 1 2 3 4; do
  for cmd in "${LIGHT[@]}"; do
    EXPECTED=$(( $(prompts) + 1 ))
    T0=$(now_ms)
    echo "$cmd" >&3
    wait_for_prompts "$EXPECTED"
    MS=$(( $(now_ms) - T0 ))
    TOTAL=$((TOTAL + 1))
    SUM=$((SUM + MS))
    [ "$MS" -le "$LIMIT_MS" ] && OK=$((OK + 1))
    [ "$MS" -gt "$SLOWEST" ] && SLOWEST=$MS
  done
done

echo "exit" >&3
exec 3>&-
wait "$PID" 2>/dev/null
PID=""

PERCENT=$(( OK * 100 / TOTAL ))
echo "First prompt after ${STARTUP_MS} ms; $OK of $TOTAL commands returned within ${LIMIT_MS} ms (${PERCENT}%), average $((SUM / TOTAL)) ms, slowest ${SLOWEST} ms."

if [ "$STARTUP_MS" -le "$LIMIT_MS" ] && [ "$PERCENT" -ge 90 ]; then
  echo "PASS TC-PERF-01  the prompt returns within 1 second (SSH-NF-001)."
  exit 0
fi
echo "FAIL TC-PERF-01  the prompt is too slow (SSH-NF-001)."
exit 1
