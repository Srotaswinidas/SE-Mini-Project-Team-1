#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

static char *history[MAX_HISTORY];
static int hist_count = 0;
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

/* Release all command history allocations. */
static void cleanup_history(void)
{
    for (int i = 0; i < hist_count; i++)
    {
        free(history[i]);
        history[i] = NULL;
    }

    hist_count = 0;
}

/* F-013 + NF-002: remember commands. When the list is full the oldest entry
 * is dropped, so long sessions keep the most recent MAX_HISTORY commands. */
static void add_to_history(const char *line)
{
    char *copy = strdup(line);
    if (copy == NULL)
        return; /* out of memory: skip, do not crash */
    if (hist_count == MAX_HISTORY)
    {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof history[0]);
        hist_count--;
    }
    history[hist_count++] = copy;
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

/* SR-004: do two paths lead to the same existing file (also through links)? */
static int same_file(const char *a, const char *b)
{
    struct stat sa, sb;
    return stat(a, &sa) == 0 && stat(b, &sb) == 0 &&
           sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

/* SR-004: the word after 'op' ("<" or ">") in a NULL-terminated argv, or NULL */
static const char *redirect_target(char **argv, const char *op)
{
    for (int i = 0; argv[i] != NULL; i++)
        if (strcmp(argv[i], op) == 0 && argv[i + 1] != NULL)
            return argv[i + 1];
    return NULL;
}

/* F-002 + SR-001 + SR-002: read one line into buf (capacity 'size').
 * Never writes more than MAX_INPUT characters plus the terminating NUL.
 * Returns 1 = ok, 0 = EOF (Ctrl+D), -1 = too long, -2 = interrupted by a signal,
 *        -3 = the line contained a NUL byte */
static int read_line(char *buf, size_t size)
{
    size_t limit = (size > 0) ? size - 1 : 0; /* room for the final NUL */
    if (limit > MAX_INPUT)
        limit = MAX_INPUT;
    size_t len = 0;
    int too_long = 0, bad_char = 0, c;

    errno = 0;
    while ((c = getchar()) != EOF && c != '\n')
    {
        if (c == '\0')
            bad_char = 1;
        else if (len < limit)
            buf[len++] = (char)c;
        else
            too_long = 1; /* keep reading, but store nothing more */
    }
    if (c == EOF)
    {
        if (ferror(stdin) && errno == EINTR)
        {
            clearerr(stdin);
            return -2;
        }
        if (len == 0 && !too_long && !bad_char)
            return 0; /* real end of input */
    }
    if (size > 0)
        buf[len] = '\0';
    if (bad_char)
        return -3;
    if (too_long)
        return -1;
    return 1;
}

/* Helper to pad spaces around |, <, and > so strtok can tokenize them cleanly */
static void normalize_operators(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 2 < dst_size; i++)
    {
        if (src[i] == '|' || src[i] == '<' || src[i] == '>')
        {
            if (j > 0 && dst[j - 1] != ' ' && dst[j - 1] != '\t')
                dst[j++] = ' ';
            dst[j++] = src[i];
            if (src[i + 1] != ' ' && src[i + 1] != '\t' && src[i + 1] != '\0')
                dst[j++] = ' ';
        }
        else
        {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

/* F-004: split the line into words. Returns word count, or -1 if malformed */
static int parse(char *line, char **argv)
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

/* SR-003: validate syntax for redirection and pipelines */
static int validate_syntax(int argc, char **argv)
{
    int pipes = 0;
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], "|") == 0)
            pipes++;
    if (pipes > 1)
        return -1; /* only a two-command pipeline is supported */

    int seen_pipe = 0;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
            seen_pipe = 1;
        else if (strcmp(argv[i], ">") == 0 && pipes == 1 && !seen_pipe)
            return -1; /* '>' before '|': that output is the pipe */
        else if (strcmp(argv[i], "<") == 0 && seen_pipe)
            return -1; /* '<' after '|': that input is the pipe */
    }
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0)
        {
            if (i == 0 || i == argc - 1)
                return -1; /* missing command or target file */
            if (strcmp(argv[i + 1], ">") == 0 || strcmp(argv[i + 1], "<") == 0 || strcmp(argv[i + 1], "|") == 0)
                return -1; /* consecutive operators */
        }
        else if (strcmp(argv[i], "|") == 0)
        {
            if (i == 0 || i == argc - 1)
                return -1; /* missing left or right command */
            if (strcmp(argv[i + 1], "|") == 0)
                return -1; /* consecutive pipes */
        }
    }
    return 0;
}

/* F-009 to F-013: built-in commands. Returns 1 if the command was a built-in */
static int run_builtin(int argc, char **argv)
{
   if (strcmp(argv[0], "exit") == 0)
{
    cleanup_history();
    exit(0);
}
    if (strcmp(argv[0], "cd") == 0)
    {
        const char *dir = (argc > 1) ? argv[1] : getenv("HOME");
        if (dir == NULL || chdir(dir) != 0)
            perror("myshell: cd");
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

/* Extracts < and > redirection targets from argv and null-terminates the command arguments */
static int extract_redirection(char **argv, char **infile, char **outfile)
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
static int apply_redirection(const char *infile, const char *outfile)
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

/* Find pipe operator in argument list */
static int find_pipe(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
            return i;
    }
    return -1;
}

/* F-016: Execute a two-command pipeline */
static void execute_pipeline(int pipe_idx, char **argv)
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
        if (errno == ENOENT)
            fprintf(stderr, "myshell: %s: command not found\n", cmd1[0]);
        else
            perror("myshell");
        _exit(127);
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
        if (errno == ENOENT)
            fprintf(stderr, "myshell: %s: command not found\n", cmd2[0]);
        else
            perror("myshell");
        _exit(127);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(p1, NULL, 0);
    wait_for_child(p2);
}

/* F-005 to F-008, F-014, F-015, SR-004: Execute a single command with redirection */
static void execute_command(int argc, char **argv)
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
        if (errno == ENOENT)
            fprintf(stderr, "myshell: %s: command not found\n", argv[0]);
        else
            perror("myshell");
        _exit(127);
    }
    wait_for_child(pid);
}

int main(void)
{
    /* Keep the shell running when Ctrl+C or Ctrl+\ is pressed */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    {
        perror("myshell: signal");
        return 1;
    }
    signal(SIGQUIT, SIG_IGN);

    char line[MAX_INPUT + 2]; 
    char norm_line[MAX_INPUT * 2 + 2];
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

        if (argc < 0 || (argc > 0 && strchr("&;", argv[0][0]) != NULL) || validate_syntax(argc, argv) != 0)
        {
            fprintf(stderr, "myshell: malformed command\n"); /* SR-003 */
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
