# Simple Shell (Command Line Interpreter)

**Problem Statement No. 18** | Software Engineering Lab-1 | UE24CS341A  
**PES University**

A simple interactive command-line shell for Linux/Unix-like systems, developed in C using POSIX system facilities. The project demonstrates how a shell reads user commands, processes input, executes programs and interacts with the operating system.

## Team Members

| Name            | Role          |
| --------------- | ------------- |
| Srotaswini Das  | Developer     |
| Sushanth Bhat   | Test Engineer |
| SRK Akash       | Product Owner |
| Tarsh Choudhary | QA Lead       |

## Project Overview

The Simple Shell provides a text-based interface through which users can enter commands. It follows a Read-Eval-Print Loop (REPL):

1. Display the shell prompt.
2. Read user input.
3. Parse the command and its arguments.
4. Execute the requested command.
5. Display output or error messages.
6. Return to the prompt.

## Features

The project scope for version 1.0 includes:

- Interactive command prompt and input handling.
- Command parsing and argument handling.
- Execution of external commands using process creation and execution facilities.
- Built-in commands: `cd`, `pwd`, `help` and `exit`.
- Session-based command history.
- Basic input (`<`) and output (`>`) redirection.
- Basic two-command pipelines (`|`).
- Foreground process execution.
- Signal handling, including Ctrl+C.
- Invalid command and error handling.
- Input validation and resource management.

Advanced shell features such as scripting, background job control, command substitution and wildcard expansion are outside the scope of version 1.0.

## Technology Stack

- **Language:** C
- **Compiler:** GCC
- **Build System:** Make
- **Operating Environment:** Linux / Unix-like systems
- **System Interfaces:** POSIX process, file and signal handling facilities.

## Repository Structure

| File / Directory                                 | Description                                    |
| ------------------------------------------------ | ---------------------------------------------- |
| `src/`                                           | Shell source code and header files             |
| `Makefile`                                       | Build, test and cleanup commands               |
| `run_all_tests.sh`                               | Test execution script                          |
| `SRS_Simple_Shell_Command_Line_Interpreter.docx` | Software Requirements Specification            |
| `Test_Plan_Simple_Shell.docx`                    | Software Test Plan                             |
| `SAD_Unix_Shell.docx`                            | Software Architecture and Design Specification |

## Build and Run

**Requirements:** GCC, Make and a Linux/Unix-like environment.

Clone the repository:

```bash
git clone https://github.com/SrotaswiniDas/SE-Mini-Project-Team-1.git
cd SE-Mini-Project-Team-1
```

Build the project:

```bash
make
```

Run the shell:

```bash
./myshell
```

Run the test script through Make:

```bash
make test
```

Build with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
make asan
```

Clean generated binaries:

```bash
make clean
```

## Testing and Verification

Testing is organized around the requirements documented in the SRS and the test cases defined in the Software Test Plan.

Testing areas include:

- Functional testing
- Integration and system testing
- Regression testing
- Performance and reliability testing
- Usability and error-handling testing
- Security and input-validation testing
- Portability and build verification

The Requirements Traceability Matrix (RTM) records the relationship between requirements and their corresponding test cases. Actual execution status and test results are maintained in the project test documentation.

## Project Documentation

The project documentation includes:

1. **Software Requirements Specification (SRS):** Defines the functional, non-functional and security requirements.
2. **Software Test Plan (STP):** Defines the testing approach, test cases, environment and verification activities.
3. **Software Architecture and Design Specification (SAD):** Describes the system architecture, components, interfaces and design decisions.
4. **Requirements Traceability Matrix (RTM):** Maps requirements to their corresponding test cases and verification status.

## Project Demonstration

**Demo Video:** [Insert the recorded project demonstration video link here]

## Project Status

The project is in its final documentation and review stage. Development is frozen for the final submission. Requirement verification status is maintained in the RTM, and the project documentation is being finalized.

## References

- POSIX.1 (IEEE Std 1003.1)
- Software Requirements Specification – Simple Shell
- Software Test Plan – Simple Shell
- Software Architecture and Design Specification – Simple Shell
