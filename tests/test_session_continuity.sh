
#!/bin/bash

set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

output=$(printf 'xyzabc\npwd\nexit\n' | ./simple_shell 2>&1)

if [[ "$output" == *"command not found"* ]] && [[ "$output" == *"$(pwd)"* ]]; then
    echo "PASS: Shell continues after an invalid command."
else
    echo "FAIL: Shell did not handle the invalid command correctly."
    echo "$output"
    exit 1
fi
