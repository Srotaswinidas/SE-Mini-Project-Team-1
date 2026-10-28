#!/bin/bash
set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

# Test 1: Malformed command rejection (SSH-SR-003 / TC-SEC-03)
output=$(printf '| ls\n> file\n< file\n& ls\nexit\n' | ./simple_shell 2>&1)
if [[ "$output" != *"myshell: malformed command"* ]]; then
    echo "FAIL: Malformed command rejection check failed"
    exit 1
fi

# Test 2: Input length validation > 1024 characters (SSH-SR-001 / TC-SEC-01)
long_str=$(python3 -c 'print("A" * 1050)')
output=$(printf "%s\npwd\nexit\n" "$long_str" | ./simple_shell 2>&1)
if [[ "$output" != *"input too long"* ]] || [[ "$output" != *"$(pwd)"* ]]; then
    echo "FAIL: Input length validation failed or shell crashed"
    exit 1
fi

echo "PASS: Security and input validation tests pass (SSH-SR-001, SSH-SR-003)."
