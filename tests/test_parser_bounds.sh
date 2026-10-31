
#!/bin/bash
set -e

# Compile with AddressSanitizer and UndefinedBehaviorSanitizer.
gcc -Wall -Wextra -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer src/shell.c -o simple_shell

# Generate an input line longer than MAX_INPUT.
{
    printf '%1100s\n' ''
    # Generate a command containing more than MAX_ARGS arguments.
    printf 'echo'
    for i in {1..70}; do
        printf ' arg%s' "$i"
    done
    printf '\n'
    printf 'exit\n'
} > boundary_input.txt

# Run the shell and capture its output.
if ./simple_shell < boundary_input.txt > boundary_output.txt 2>&1; then
    :
else
    echo "FAIL: Shell exited with an error"
    cat boundary_output.txt
    exit 1
fi

# Verify that the oversized input is rejected.
if ! grep -q "input too long" boundary_output.txt; then
    echo "FAIL: Long input was not rejected"
    cat boundary_output.txt
    exit 1
fi

# Verify that excessive arguments are rejected.
if ! grep -q "malformed command" boundary_output.txt; then
    echo "FAIL: Too many arguments were not rejected"
    cat boundary_output.txt
    exit 1
fi

# Verify that sanitizers did not report memory errors.
if grep -Eq "ERROR: AddressSanitizer|runtime error:" boundary_output.txt; then
    echo "FAIL: Sanitizer detected a memory error"
    cat boundary_output.txt
    exit 1
fi

echo "PASS: Boundary tests completed without reported memory errors."

rm -f simple_shell boundary_input.txt boundary_output.txt
