/* shell.h - settings shared by all modules of the simple shell.
 *
 * Module overview (the names follow the RTM in the SRS):
 *   main.c      UI / Main Loop    prompt, read-parse-run loop           F-001, F-003, F-018
 *   input.c     Input Module      reading a line safely                 F-002, SR-001, SR-002
 *   parser.c    Parser Module     words, syntax checks, error reasons   F-004, SR-003, NF-003
 *   builtins.c  Built-in Module   cd, pwd, help, history, exit          F-009 to F-013
 *   history.c   History Module    the list of earlier commands          F-013, NF-002, SR-005
 *   redirect.c  I/O Module        '<' and '>' files, safety checks      F-014, F-015, SR-004
 *   executor.c  Execution Module  running commands and two-command pipes F-005 to F-008, F-016, SR-005
 *   signals.c   Signal Module     Ctrl+C and Ctrl+\                     F-017
 */
#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT   1024                 /* longest accepted command line (SR-001) */
#define MAX_ARGS    64                   /* a command line may have at most MAX_ARGS - 1 words (SR-002) */
#define MAX_HISTORY 100                  /* the newest MAX_HISTORY commands are remembered (NF-002) */
#define NORM_SIZE   (MAX_INPUT * 2 + 2)  /* room for a line after spaces were added around operators */

#endif
