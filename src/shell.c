/* shell.c - single-file build of the shell.
 *
 * The real code lives in the modules next to this file (see shell.h for the list).
 * This file only includes them, so the shell can also be built with
 *     gcc -Wall -Wextra src/shell.c -o simple_shell
 * which is what the test scripts and the unit tests do. 'make' builds the modules
 * one by one instead. Do not add code here: change the module files. */
#include "history.c"
#include "input.c"
#include "parser.c"
#include "builtins.c"
#include "redirect.c"
#include "signals.c"
#include "executor.c"
#include "main.c"
