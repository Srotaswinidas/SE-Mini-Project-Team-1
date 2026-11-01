/* builtins.c - Built-in Module (F-009 to F-013).
 * exit, cd, pwd, help and history. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "shell.h"
#include "history.h"
#include "builtins.h"

/* F-009 to F-013: built-in commands. Returns 1 if the command was a built-in */
int run_builtin(int argc, char **argv)
{
    if (strcmp(argv[0], "exit") == 0)
    {
        cleanup_history();
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0)
    {
        if (argc > 2)
        {
            fprintf(stderr, "myshell: cd: too many arguments\n"); /* NF-003 */
            return 1;
        }
        const char *dir = (argc > 1) ? argv[1] : getenv("HOME");
        if (dir == NULL)
            fprintf(stderr, "myshell: cd: HOME is not set\n");
        else if (chdir(dir) != 0)
            fprintf(stderr, "myshell: cd: %s: %s\n", dir, strerror(errno));
        return 1;
    }
    if (strcmp(argv[0], "pwd") == 0)
    {
        char cwd[4096];
        if (getcwd(cwd, sizeof cwd) != NULL)
            printf("%s\n", cwd);
        else
            perror("myshell: pwd");
        return 1;
    }
    if (strcmp(argv[0], "help") == 0)
    {
        printf("Built-in commands:\n");
        printf("  cd [dir]   change directory\n");
        printf("  pwd        print current directory\n");
        printf("  help       show this help\n");
        printf("  history    show earlier commands\n");
        printf("  exit       leave the shell\n");
        printf("Any other command is run from PATH, e.g. ls -l\n");
        return 1;
    }
    if (strcmp(argv[0], "history") == 0)
    {
        for (int i = 0; i < hist_count; i++)
            printf("%4d  %s\n", i + 1, history[i]);
        return 1;
    }
    return 0;
}
