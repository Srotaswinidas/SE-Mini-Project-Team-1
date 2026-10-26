#!/bin/bash
set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

output=$(printf 'help\npwd\nhistory\nexit\n' | ./simple_shell 2>&1)

[[ "$output" == *"Built-in commands:"* ]] || { echo "FAIL: help"; exit 1; }
[[ "$output" == *"$(pwd)"* ]] || { echo "FAIL: pwd"; exit 1; }
[[ "$output" == *"help"* ]] || { echo "FAIL: history"; exit 1; }

echo "PASS: Built-in commands work."
