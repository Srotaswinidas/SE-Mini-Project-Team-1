/* main.c - UI / Main Loop Module (F-001, F-003, F-018).
 * Prints the prompt, reads a line, checks it and runs it, again and again.
 * An error never ends the loop, so the shell stays usable after a mistake. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "shell.h"
#include "input.h"
#include "parser.h"
#include "history.h"
#include "signals.h"
#include "executor.h"

/* F-001, F-003, F-018: the read-check-run loop. Every error is reported and the loop goes on;
 * the shell ends only with the exit command or at the end of the input (Ctrl+D). */
int main(void)
{
    if (setup_signals() != 0) /* F-017: Ctrl+C and Ctrl+\ must not stop the shell */
        return 1;

    char line[MAX_INPUT + 2];
    char norm_line[NORM_SIZE];
    char copy[MAX_INPUT + 2];
    char *argv[MAX_ARGS];

    while (1)
    {
        printf("myshell> "); /* F-001 */
        fflush(stdout);

        int r = read_line(line, sizeof line);
        if (r == 0)
        {
            printf("\n");
            cleanup_history();
            break;
        }
        if (r == -2)
            continue; /* interrupted: prompt again */
        if (r == -1)
        {
            fprintf(stderr, "myshell: input too long (max %d characters)\n", MAX_INPUT);
            continue;
        }
        if (r == -3)
        {
            fprintf(stderr, "myshell: input contains an invalid character\n"); /* SR-002 */
            continue;
        }

        snprintf(copy, sizeof copy, "%s", line);
        normalize_operators(line, norm_line, sizeof norm_line);
        int argc = parse(norm_line, argv);

        if (argc < 0)
        {
            fprintf(stderr, "myshell: malformed command: too many words (at most %d)\n", MAX_ARGS - 1);
            continue;
        }
        const char *problem = (argc > 0) ? syntax_problem(argc, argv) : NULL;
        if (problem != NULL)
        {
            fprintf(stderr, "myshell: malformed command: %s\n", problem); /* SR-003, NF-003 */
            continue;
        }
        if (argc == 0)
            continue; /* F-003 */

        add_to_history(copy);

        int pipe_idx = find_pipe(argc, argv);
        if (pipe_idx != -1)
            execute_pipeline(pipe_idx, argv);
        else
            execute_command(argc, argv);
    }
    return 0;
}
