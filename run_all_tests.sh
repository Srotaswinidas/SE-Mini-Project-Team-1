#!/bin/bash
# Master test runner for Simple Shell test suites (QA Lead Deliverable)
set -e

echo "========================================="
echo "Running Simple Shell Test Suite"
echo "========================================="

bash tests/test_builtins.sh
bash tests/test_commands.sh
bash tests/test_session_continuity.sh
bash tests/test_security.sh
bash tests/test_reliability.sh
bash tests/test_redirection.sh
bash tests/test_pipes.sh

echo "========================================="
echo "All QA and Functional Test Suites PASSED!"
echo "========================================="
