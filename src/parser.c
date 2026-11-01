/* parser.c - Parser Module (F-004, SR-003, NF-003).
 * Splits a line into words and explains what is wrong with a malformed one. */
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "shell.h"
#include "parser.h"

/* F-004: split the line into words. Returns word count, or -1 if malformed */
int parse(char *line, char **argv)
{
    int argc = 0;
    char *tok = strtok(line, " \t");
    while (tok != NULL)
    {
        if (argc >= MAX_ARGS - 1)
            return -1; /* too many arguments */
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    return argc;
}

/* SR-003 + NF-003: check the syntax of redirections and pipelines.
 * Returns NULL if the command is fine, otherwise a short reason to show the user. */
const char *syntax_problem(int argc, char **argv)
{
    if (argc > 0 && strchr("&;", argv[0][0]) != NULL)
        return "'&' and ';' are not supported";
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], ";") == 0 || strcmp(argv[i], "&") == 0)
            return "'&' and ';' are not supported";

    /* a missing command or file name next to an operator */
    for (int i = 0; i < argc; i++)
    {
        int is_in = (strcmp(argv[i], "<") == 0);
        int is_out = (strcmp(argv[i], ">") == 0);
        int is_pipe = (strcmp(argv[i], "|") == 0);
        if (is_in || is_out)
        {
            if (i == 0)
                return is_in ? "no command before '<'" : "no command before '>'";
            if (i == argc - 1 || strcmp(argv[i + 1], ">") == 0 ||
                strcmp(argv[i + 1], "<") == 0 || strcmp(argv[i + 1], "|") == 0)
                return is_in ? "missing file name after '<'" : "missing file name after '>'";
        }
        else if (is_pipe)
        {
            if (i == 0)
                return "no command before '|'";
            if (i == argc - 1)
                return "no command after '|'";
            if (strcmp(argv[i + 1], "|") == 0)
                return "empty command between two '|'";
        }
    }

    /* only a two-command pipeline is supported, with '<' first and '>' last */
    int pipes = 0;
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], "|") == 0)
            pipes++;
    if (pipes > 1)
        return "only two commands can be joined with '|'";

    int seen_pipe = 0;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
            seen_pipe = 1;
        else if (strcmp(argv[i], ">") == 0 && pipes == 1 && !seen_pipe)
            return "'>' is only allowed on the last command of a pipeline";
        else if (strcmp(argv[i], "<") == 0 && seen_pipe)
            return "'<' is only allowed on the first command of a pipeline";
    }
    return NULL;
}

/* Find pipe operator in argument list */
int find_pipe(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
            return i;
    }
    return -1;
}
