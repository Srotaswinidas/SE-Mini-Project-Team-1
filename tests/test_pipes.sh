#!/bin/bash
set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

# Test 1: Pipeline ls | grep src (F-016 / TC-SH-16)
output=$(printf "ls | grep src\nexit\n" | ./simple_shell 2>&1)
if [[ "$output" != *"src"* ]]; then
    echo "FAIL: Two-command pipeline (ls | grep src) failed"
    exit 1
fi

# Test 2: Pipeline with word count
line_count=$(printf "ls | wc -l\nexit\n" | ./simple_shell 2>&1 | tr -dc '0-9')
if [ -z "$line_count" ] || [ "$line_count" -le 0 ]; then
    echo "FAIL: Pipeline ls | wc -l produced invalid count"
    exit 1
fi

echo "PASS: Two-command pipeline tests pass (F-016 / TC-SH-16)."
