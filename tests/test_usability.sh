#!/bin/bash
# TC-USAB-01 (SSH-NF-003): error messages are understandable and the shell stays active.
# Every case sends one bad command, then "echo still-active": the shell must still answer,
# the error must say what is wrong, and every error line must start with "myshell:".
# Run from the repo root:  bash tests/test_usability.sh

set -u

# Build the shell with warnings enabled.
gcc -Wall -Wextra src/shell.c -o simple_shell || { echo "FAIL: the shell does not compile"; exit 1; }
SHELL_BIN=./simple_shell

DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
pass=0
fail=0

report() {      # report <ok 0/1> <description> <detail>
  if [ "$1" -eq 0 ]; then echo "PASS TC-USAB-01  $2"; pass=$((pass + 1))
  else echo "FAIL TC-USAB-01  $2  ($3)"; fail=$((fail + 1)); fi
}

# run_case <description> <input line> <text that must appear in the error output>
run_case() {
  local desc=$1 input=$2 expected=$3
  printf '%b\necho still-active\nexit\n' "$input" | timeout 10 "$SHELL_BIN" >"$DIR/out" 2>"$DIR/err"
  if ! grep -qF -- "$expected" "$DIR/err"; then
    report 1 "$desc" "expected the message: $expected | got: $(head -c 150 "$DIR/err")"; return
  fi
  if ! grep -qF "still-active" "$DIR/out"; then
    report 1 "$desc" "the shell stopped after the error"; return
  fi
  if grep -qv '^myshell: ' "$DIR/err"; then
    report 1 "$desc" "an error line does not start with 'myshell:'"; return
  fi
  report 0 "$desc"
}

words() { local i out=""; for ((i = 0; i < $1; i++)); do out+=" w"; done; echo "$out"; }

printf '#!/bin/sh\necho hi\n' > "$DIR/noexec"
chmod 644 "$DIR/noexec"

# --- commands that cannot run ---
run_case "unknown command names the command"        "nosuchcmd"                       "myshell: nosuchcmd: command not found"
run_case "file without execute permission"          "$DIR/noexec"                     "$DIR/noexec: Permission denied"
run_case "unknown command inside a pipeline"        "nosuchcmd | cat"                 "nosuchcmd: command not found"

# --- cd ---
run_case "cd to a missing directory names it"       "cd /no/such/dir"                 "myshell: cd: /no/such/dir: No such file or directory"
run_case "cd with too many arguments"               "cd a b"                          "myshell: cd: too many arguments"

# --- input problems ---
run_case "input that is too long"                   "$(head -c 1100 /dev/zero | tr '\0' a)" "input too long (max 1024 characters)"
run_case "NUL byte in the input"                    'echo a\0b'                       "input contains an invalid character"
run_case "too many words"                           "echo$(words 63)"                 "too many words (at most 63)"

# --- redirection and pipeline syntax ---
run_case "'>' without a file name"                  "echo hi >"                       "missing file name after '>'"
run_case "'<' without a file name"                  "cat <"                           "missing file name after '<'"
run_case "'>' without a command"                    "> out.txt"                       "no command before '>'"
run_case "'|' without a command before it"          "| ls"                            "no command before '|'"
run_case "'|' without a command after it"           "ls |"                            "no command after '|'"
run_case "two '|' in a row"                         "ls | | wc"                       "empty command between two '|'"
run_case "three commands in a pipeline"             "echo a | cat | cat"              "only two commands can be joined with '|'"
run_case "'>' before a pipe"                        "echo a > $DIR/f | cat"           "'>' is only allowed on the last command"
run_case "'<' after a pipe"                         "echo a | cat < $DIR/f"           "'<' is only allowed on the first command"
run_case "unsupported ';'"                          "ls ; pwd"                        "'&' and ';' are not supported"
run_case "unsupported '&'"                          "sleep 1 &"                       "'&' and ';' are not supported"

# --- files that cannot be opened ---
run_case "missing input file names the file"        "cat < /no/such/file"             "myshell: /no/such/file: No such file or directory"
run_case "output file in a missing directory"       "echo hi > /no/such/dir/out"      "/no/such/dir/out: No such file or directory"
run_case "directory as output file"                 "echo hi > $DIR"                  "Is a directory"

# --- cd without HOME ---
printf 'cd\necho still-active\nexit\n' | env -u HOME timeout 10 "$SHELL_BIN" >"$DIR/out" 2>"$DIR/err"
if grep -qF "myshell: cd: HOME is not set" "$DIR/err" && grep -qF "still-active" "$DIR/out"; then report 0 "cd without HOME says so"
else report 1 "cd without HOME says so" "message or shell missing"; fi

echo
echo "Passed: $pass   Failed: $fail"
[ "$fail" -eq 0 ]
