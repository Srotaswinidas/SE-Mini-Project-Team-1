# Simple Shell (Command Line Interpreter)

**Problem Statement No. 18** | Software Engineering Lab | UE24CS341A, PES University

A simple interactive shell for Linux/Unix-like systems, written in C using standard POSIX APIs. It accepts commands from the user, parses them, and executes them through the operating system. It is an educational project and not a replacement for bash or zsh.

## Team

| Name | Role |
| --- | --- |
| Srotaswini Das | Developer |
| Sushant Bhat | Test Engineer |
| SRK Akash | Product Owner |
| Tarsh Choudhary | QA Lead |


## Repository Contents

| Deliverable | File | Status |
| --- | --- | --- |
| Software Requirements Specification (SRS) v1.0 | `SRS_Simple_Shell_Command_Line_Interpreter.docx` | Submitted |
| Software Test Plan (STP) v1.0 | `Test_Plan_Simple_Shell.docx` | Submitted |

The architecture document is prepared using the template provided by the instructor.

## Project Overview

The shell repeatedly runs this cycle: display a prompt, read a command line, parse it, execute it, wait for completion, show results or errors, and return to the prompt.

### Planned Features

Each feature maps to a requirement ID in the SRS (Section 4).

| Area | Features | Requirement IDs |
| --- | --- | --- |
| Prompt and input | Prompt display, reading a command line, ignoring empty input, argument parsing | SSH-F-001 to 004 |
| Execution | External commands via fork/exec, PATH lookup, waiting for foreground processes, invalid command handling | SSH-F-005 to 008 |
| Built-ins | `cd`, `pwd`, `help`, `exit` | SSH-F-009 to 012 |
| History | Command history for the current session | SSH-F-013 |
| I/O | Output redirection (`>`), input redirection (`<`), two-command pipelines (`\|`) | SSH-F-014 to 016 |
| Robustness | Signal handling (Ctrl+C), session continuity after errors | SSH-F-017, 018 |

### Non-Functional and Security Requirements

- **Performance:** the prompt returns within 1 second for lightweight commands (90% of runs) (SSH-NF-001)
- **Reliability:** the shell survives at least 100 consecutive valid and invalid commands (SSH-NF-002)
- **Usability, maintainability and portability:** SSH-NF-003 to 005
- **Security:** input length validation, bounds-safe parsing, rejection of malformed commands, safe handling of redirection failures, and resource cleanup (SSH-SR-001 to 005)

In total there are 28 requirements: 18 functional, 5 non-functional and 5 security.

## Architecture Summary

The planned modules below come from the RTM in the SRS (Section 8). The detailed design is in the architecture document.

| Module | Responsibility |
| --- | --- |
| UI Module | Prompt display |
| Input Module | Reading command lines and length validation |
| Parser | Tokenising commands and arguments, rejecting malformed input |
| Execution Module | PATH lookup, fork/exec of external commands |
| Process Module | Waiting for foreground processes |
| Built-in Module | `cd`, `pwd`, `help`, `exit` |
| History Module | Session command history |
| I/O Module | Input and output redirection |
| Pipe Module | Two-command pipelines |
| Signal Module | Interrupt and termination handling |
| Error Module / Main Loop | Error reporting and session continuity |

## Technology and Environment

- **Language:** C (gcc with `-Wall -Wextra`)
- **APIs:** POSIX (`fork`, `exec`, `wait`/`waitpid`, `pipe`, `dup2`, `chdir`, `getcwd`, signals)
- **Target platform:** Linux/Unix-like. The test plan uses Ubuntu 22.04 LTS or later (native or WSL2).
- **Build:** `make`

## Testing

Testing follows the Software Test Plan.

- **Levels:** unit, integration, system and acceptance
- **Types:** functional, regression, performance, reliability, usability, security and portability
- **Tools:** Valgrind, AddressSanitizer/UndefinedBehaviorSanitizer, gdb, strace, lsof, cppcheck, clang-tidy, bash scripts and expect
- **Traceability:** every requirement has at least one test case, listed in the SRS RTM and in STP Section 13 (TC-SH-01 to 18, TC-PERF-01, TC-REL-01, TC-USAB-01, TC-MAINT-01, TC-PORT-01, TC-SEC-01 to 05)

## Project Status

| Item | Status |
| --- | --- |
| SRS | Done |
| Test Plan | Done |
| Architecture | In progress |
| Implementation | Not started (all RTM entries currently marked N) |



## References

- Simple Shell SRS v1.0
- Simple Shell Software Test Plan v1.0
- POSIX.1 (IEEE Std 1003.1)
- IEEE 829 test documentation outline