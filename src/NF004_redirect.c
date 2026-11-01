/* redirect.c - I/O Module (F-014, F-015, SR-004).
 * Finds "< file" and "> file" in a command and connects them to stdin and stdout. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "shell.h"
#include "redirect.h"

/* SR-004: do two paths lead to the same existing file (also through links)? */
int same_file(const char *a, const char *b)
{
    struct stat sa, sb;
    return stat(a, &sa) == 0 && stat(b, &sb) == 0 &&
           sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

/* SR-004: the word after 'op' ("<" or ">") in a NULL-terminated argv, or NULL */
const char *redirect_target(char **argv, const char *op)
{
    for (int i = 0; argv[i] != NULL; i++)
        if (strcmp(argv[i], op) == 0 && argv[i + 1] != NULL)
            return argv[i + 1];
    return NULL;
}

/* Extracts < and > redirection targets from argv and null-terminates the command arguments */
int extract_redirection(char **argv, char **infile, char **outfile)
{
    *infile = NULL;
    *outfile = NULL;
    int j = 0;
    for (int i = 0; argv[i] != NULL; i++)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            if (argv[i + 1] == NULL)
                return -1;
            *infile = argv[++i];
        }
        else if (strcmp(argv[i], ">") == 0)
        {
            if (argv[i + 1] == NULL)
                return -1;
            *outfile = argv[++i];
        }
        else
        {
            argv[j++] = argv[i];
        }
    }
    argv[j] = NULL;
    return j;
}

/* F-014, F-015, SR-004: Apply file redirection safely */
int apply_redirection(const char *infile, const char *outfile)
{
    if (infile != NULL)
    {
        int fd_in = open(infile, O_RDONLY);
        if (fd_in < 0)
        {
            fprintf(stderr, "myshell: %s: %s\n", infile, strerror(errno));
            return -1;
        }
        if (dup2(fd_in, STDIN_FILENO) < 0)
        {
            perror("myshell: dup2 stdin");
            close(fd_in);
            return -1;
        }
        close(fd_in);
    }
    if (outfile != NULL)
    {
        int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0)
        {
            fprintf(stderr, "myshell: %s: %s\n", outfile, strerror(errno));
            return -1;
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0)
        {
            perror("myshell: dup2 stdout");
            close(fd_out);
            return -1;
        }
        close(fd_out);
    }
    return 0;
}
