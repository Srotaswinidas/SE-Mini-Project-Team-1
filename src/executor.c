/* executor.c - Execution Module (F-005 to F-008, F-016, SR-005, NF-003).
 * Starts child processes, connects pipes and redirections, waits for the children
 * and explains it when a command cannot be started. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "shell.h"
#include "builtins.h"
#include "redirect.h"
#include "executor.h"

/* Retry waiting if a signal interrupts waitpid. */
static void wait_for_child(pid_t pid)
{
    while (waitpid(pid, NULL, 0) < 0)
    {
        if (errno == EINTR)
            continue;

        if (errno != ECHILD)
            perror("myshell: waitpid");

        break;
    }
}

/* NF-003: explain why a command could not be started.
 * Returns the exit code for the child: 127 = not found, 126 = found but not runnable. */
static int report_exec_failure(const char *cmd)
{
    int err = errno;
    if (err == ENOENT)
    {
        fprintf(stderr, "myshell: %s: command not found\n", cmd);
        return 127;
    }
    fprintf(stderr, "myshell: %s: %s\n", cmd, strerror(err));
    return 126;
}

/* SR-005: a child must not inherit descriptors the shell happens to have open.
 * Close everything above stdin/stdout/stderr before exec. */
static void close_inherited_fds(void)
{
    long max = sysconf(_SC_OPEN_MAX);
    if (max < 0 || max > 4096)
        max = 4096;
    for (int fd = 3; fd < max; fd++)
        close(fd);
}

/* F-016: Execute a two-command pipeline */
void execute_pipeline(int pipe_idx, char **argv)
{
    argv[pipe_idx] = NULL;
    char **cmd1 = argv;
    char **cmd2 = &argv[pipe_idx + 1];

    const char *in_name = redirect_target(cmd1, "<");
    const char *out_name = redirect_target(cmd2, ">");
    if (in_name != NULL && out_name != NULL && same_file(in_name, out_name))
    {
        fprintf(stderr, "myshell: %s: input and output are the same file\n", out_name);
        return; /* SR-004: it would empty the input before it is read */
    }
    fflush(stdout); /* the children must not inherit unflushed output */

    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        perror("myshell: pipe");
        return;
    }

    pid_t p1 = fork();
    if (p1 < 0)
    {
        perror("myshell: fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (p1 == 0)
    {
                if (signal(SIGINT, SIG_DFL) == SIG_ERR)
            _exit(1);
        signal(SIGQUIT, SIG_DFL);

        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char *infile = NULL, *outfile = NULL;
        int c1_argc = extract_redirection(cmd1, &infile, &outfile);
        if (c1_argc <= 0 || apply_redirection(infile, outfile) < 0)
            _exit(1);

        if (run_builtin(c1_argc, cmd1))
        {
            fflush(stdout); /* _exit() does not flush: without this the output is lost */
            _exit(0);
        }

        close_inherited_fds(); /* SR-005 */
        execvp(cmd1[0], cmd1);
        _exit(report_exec_failure(cmd1[0]));
    }

    pid_t p2 = fork();
    if (p2 < 0)
    {
        perror("myshell: fork");
        close(pipefd[0]);
        close(pipefd[1]);
        wait_for_child(p1);
        return;
    }
    if (p2 == 0)
    {
                if (signal(SIGINT, SIG_DFL) == SIG_ERR)
            _exit(1);
        signal(SIGQUIT, SIG_DFL);

        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        char *infile = NULL, *outfile = NULL;
        int c2_argc = extract_redirection(cmd2, &infile, &outfile);
        if (c2_argc <= 0 || apply_redirection(infile, outfile) < 0)
            _exit(1);

        if (run_builtin(c2_argc, cmd2))
        {
            fflush(stdout);
            _exit(0);
        }

        close_inherited_fds(); /* SR-005 */
        execvp(cmd2[0], cmd2);
        _exit(report_exec_failure(cmd2[0]));
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(p1, NULL, 0);
    wait_for_child(p2);
}

/* F-005 to F-008, F-014, F-015, SR-004: Execute a single command with redirection */
void execute_command(int argc, char **argv)
{
    (void)argc;
    char *infile = NULL;
    char *outfile = NULL;
    int new_argc = extract_redirection(argv, &infile, &outfile);
    if (new_argc <= 0)
        return;

    if (infile != NULL && outfile != NULL && same_file(infile, outfile))
    {
        fprintf(stderr, "myshell: %s: input and output are the same file\n", outfile);
        return; /* SR-004: it would empty the input before it is read */
    }

    /* Built-in without redirection runs in parent */
    if (infile == NULL && outfile == NULL)
    {
        if (run_builtin(new_argc, argv))
            return;
    }
    else if (strcmp(argv[0], "cd") == 0 || strcmp(argv[0], "exit") == 0)
    {
        run_builtin(new_argc, argv);
        return;
    }

    fflush(stdout); /* the child must not inherit unflushed output */
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("myshell: fork");
        return;
    }
    if (pid == 0)
    {
        if (signal(SIGINT, SIG_DFL) == SIG_ERR)
            _exit(1);
        signal(SIGQUIT, SIG_DFL);

        if (apply_redirection(infile, outfile) < 0)
            _exit(1);

        if (run_builtin(new_argc, argv))
        {
            fflush(stdout);
            _exit(0);
        }

        close_inherited_fds(); /* SR-005 */
        execvp(argv[0], argv);
        _exit(report_exec_failure(argv[0]));
    }
    wait_for_child(pid);
}
