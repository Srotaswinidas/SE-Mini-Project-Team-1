#!/bin/bash
set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

# Test 50 valid commands and 50 invalid commands.
commands=()

for i in {1..50}; do
    commands+=("pwd")
    commands+=("invalid_cmd_$i")
done

commands+=("exit")

# Execute all commands and capture output.
output=$(printf '%s\n' "${commands[@]}" | ./simple_shell 2>&1)

# Count successful pwd outputs and command-not-found errors.

valid_count=$(printf '%s\n' "$output" |
    grep -Fc -- "$PWD" || true)


invalid_count=$(printf '%s\n' "$output" |
    grep -Fc "command not found" || true)

if [[ "$valid_count" -eq 50 ]] &&
   [[ "$invalid_count" -eq 50 ]]; then
    echo "PASS: Shell survived 100 valid/invalid commands."
else
    echo "FAIL: Reliability test failed."
    echo "Expected valid commands: 50; got: $valid_count"
    echo "Expected invalid commands: 50; got: $invalid_count"
    exit 1
fi
