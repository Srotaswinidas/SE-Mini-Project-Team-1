#!/bin/bash
set -e

# Compile the shell with warnings enabled.
gcc -Wall -Wextra src/shell.c -o simple_shell

# Test: 100 consecutive valid and invalid commands (SSH-NF-002 / TC-REL-01)
# 50 valid commands (pwd) and 50 invalid commands, then a command that must still work.
commands=()

for i in {1..50}; do
    commands+=("pwd")
    commands+=("invalid_cmd_$i")
done

commands+=("echo reliability-done")
commands+=("exit")

# Send all commands to the shell and capture its output.
output=$(printf '%s\n' "${commands[@]}" | ./simple_shell 2>&1)

# Count successful pwd outputs and command-not-found errors.
valid_count=$(printf '%s\n' "$output" |
    grep -Fc -- "$PWD" || true)

invalid_count=$(printf '%s\n' "$output" |
    grep -Fc "command not found" || true)

done_count=$(printf '%s\n' "$output" |
    grep -Fc "reliability-done" || true)

if [[ "$valid_count" -eq 50 ]] &&
   [[ "$invalid_count" -eq 50 ]] &&
   [[ "$done_count" -ge 1 ]]; then
    echo "PASS: Shell survived 100 valid/invalid commands (SSH-NF-002)."
else
    echo "FAIL: Reliability test failed."
    echo "Expected valid commands: 50; got: $valid_count"
    echo "Expected invalid commands: 50; got: $invalid_count"
    echo "Shell still answering after the 100 commands: $done_count (expected at least 1)"
    exit 1
fi
