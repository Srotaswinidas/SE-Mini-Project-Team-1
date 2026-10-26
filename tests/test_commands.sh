#!/bin/bash
set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

output=$(printf 'cd /\npwd\nls\nexit\n' | ./simple_shell 2>&1)

[[ "$output" == *"myshell> /"* ]] || { echo "FAIL: cd or pwd"; exit 1; }
[[ "$output" == *"bin"* ]] || { echo "FAIL: external command ls"; exit 1; }

echo "PASS: Directory changing and external commands work."

