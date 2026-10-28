#!/bin/bash
set -e

gcc -Wall -Wextra src/shell.c -o simple_shell

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

OUT_FILE="$TMP_DIR/output.txt"
IN_FILE="$TMP_DIR/input.txt"

# Prepare test input
echo "Hello Simple Shell Redirection" > "$IN_FILE"

# Test 1: Output redirection > (F-014 / TC-SH-14)
printf "echo 'testing output redirection' > %s\nexit\n" "$OUT_FILE" | ./simple_shell > /dev/null 2>&1
if [[ ! -f "$OUT_FILE" ]] || ! grep -q "testing output redirection" "$OUT_FILE"; then
    echo "FAIL: Output redirection > failed to write content to file"
    exit 1
fi

# Test 2: Input redirection < (F-015 / TC-SH-15)
output=$(printf "cat < %s\nexit\n" "$IN_FILE" | ./simple_shell 2>&1)
if [[ "$output" != *"Hello Simple Shell Redirection"* ]]; then
    echo "FAIL: Input redirection < failed to read from file"
    exit 1
fi

# Test 3: Safe redirection failures (SR-004 / TC-SEC-04)
# Missing input file
fail_output=$(printf "cat < %s/nonexistent_file.txt\npwd\nexit\n" "$TMP_DIR" | ./simple_shell 2>&1)
if [[ "$fail_output" != *"No such file or directory"* ]] && [[ "$fail_output" != *"myshell:"* ]]; then
    echo "FAIL: Safe redirection failure did not report error for missing file"
    exit 1
fi

# Protected / invalid output directory
invalid_out_output=$(printf "echo test > /nonexistent_directory_xyz/out.txt\npwd\nexit\n" | ./simple_shell 2>&1)
if [[ "$invalid_out_output" != *"No such file or directory"* ]] && [[ "$invalid_out_output" != *"myshell:"* ]]; then
    echo "FAIL: Safe redirection failure did not report error for invalid output path"
    exit 1
fi

# Verify shell stayed alive and ran pwd after failures
if [[ "$fail_output" != *"$(pwd)"* ]] || [[ "$invalid_out_output" != *"$(pwd)"* ]]; then
    echo "FAIL: Shell did not continue session after redirection failure"
    exit 1
fi

echo "PASS: Output redirection (>), input redirection (<), and safe failure handling (SR-004) all pass."
