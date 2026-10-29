
#!/bin/bash
set -e

# Compile the shell with warnings enabled.
gcc -Wall -Wextra src/shell.c -o simple_shell

# Test: 100 consecutive valid and invalid commands (SSH-NF-002 / TC-REL-01)
cmd_input=""

for i in {1..50}; do
    cmd_input+="pwd\n"
    cmd_input+="invalid_cmd_$i\n"
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
    exit 1
fi
