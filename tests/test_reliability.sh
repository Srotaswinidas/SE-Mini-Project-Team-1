
#!/bin/bash
set -e

# Compile the shell with warnings enabled.
gcc -Wall -Wextra src/shell.c -o simple_shell

# Test: 100 consecutive valid and invalid commands (SSH-NF-002 / TC-REL-01)
cmd_input=""
# Test 50 valid commands and 50 invalid commands.
commands=()

for i in {1..50}; do
    commands+=("pwd")
    commands+=("invalid_cmd_$i")
done

cmd_input+="exit\n"

# Send all commands to the shell and capture its output.
output=$(printf "%b" "$cmd_input" | ./simple_shell 2>&1)

# Verify that valid commands ran and invalid commands were handled.
if [[ "$output" == *"command not found"* ]] && [[ "$output" == *"$(pwd)"* ]]; then
    echo "PASS: Shell survived 100 consecutive valid/invalid commands (SSH-NF-002)."
else
    echo "FAIL: Reliability test failed"
    echo "Captured output:"
    echo "$output"

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
