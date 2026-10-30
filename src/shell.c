#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

static char *history[MAX_HISTORY];
static int hist_count = 0;

/* F-013: remember commands */
static void add_to_history(const char *line)
{
    if (hist_count < MAX_HISTORY)
    {
        char *copy = strdup(line);
        if (copy != NULL)
            history[hist_count++] = copy;
    }
}

/* F-002 + SR-001: read a line. Returns 1 = ok, 0 = EOF (Ctrl+D), -1 = too long */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[len - 1] = '\0';
        return 1;
    }
    if (feof(stdin))
        return 1; /* last line without newline */
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    } /* throw away the rest */
    return -1;
}

/* Helper to pad spaces around |, <, and > so strtok can tokenize them cleanly */

/* Add spaces around operators without overflowing the destination buffer. */
static void normalize_operators(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;

    if (src == NULL || dst == NULL || dst_size == 0)
        return;

    for (size_t i = 0; src[i] != '\0'; i++)
    {
        if (src[i] == '|' || src[i] == '<' || src[i] == '>')
        {
            if (j > 0 && dst[j - 1] != ' ' && dst[j - 1] != '\t')
            {
                if (j + 1 >= dst_size)
                    break;
                dst[j++] = ' ';
            }

            if (j + 1 >= dst_size)
                break;
            dst[j++] = src[i];

            if (src[i + 1] != ' ' &&
                src[i + 1] != '\t' &&
                src[i + 1] != '\0')
            {
                if (j + 1 >= dst_size)
                    break;
                dst[j++] = ' ';
            }
        }
        else
        {
            if (j + 1 >= dst_size)
                break;
            dst[j++] = src[i];
        }
    }

    dst[j] = '\0';
}

/* SR-003: validate syntax for redirection and pipelines */
static int validate_syntax(int argc, char **argv)
{
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
        for (int i = 0; i < hist_count; i++)
            free(history[i]);
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
        
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char *infile = NULL, *outfile = NULL;
        int c1_argc = extract_redirection(cmd1, &infile, &outfile);
        if (c1_argc <= 0 || apply_redirection(infile, outfile) < 0)
            _exit(1);

        if (run_builtin(c1_argc, cmd1))
            _exit(0);

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
        waitpid(p1, NULL, 0);
        return;
    }
    if (p2 == 0)
    {
                if (signal(SIGINT, SIG_DFL) == SIG_ERR)
            _exit(1);
        
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        char *infile = NULL, *outfile = NULL;
        int c2_argc = extract_redirection(cmd2, &infile, &outfile);
        if (c2_argc <= 0 || apply_redirection(infile, outfile) < 0)
            _exit(1);

        if (run_builtin(c2_argc, cmd2))
            _exit(0);

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
    waitpid(p2, NULL, 0);
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
        
        if (apply_redirection(infile, outfile) < 0)
            _exit(1);

        if (run_builtin(new_argc, argv))
            _exit(0);

        execvp(argv[0], argv);
        if (errno == ENOENT)
            fprintf(stderr, "myshell: %s: command not found\n", argv[0]);
        else
            perror("myshell");
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
}
/* F-004: Split the line into words safely. */
static int parse(char *line, char **argv)
{
    int argc = 0;

    if (line == NULL || argv == NULL)
        return -1;

    argv[0] = NULL;

    char *tok = strtok(line, " \t");

    while (tok != NULL)
    {
        if (argc >= MAX_ARGS - 1)
        {
            argv[argc] = NULL;
            return -1;
        }

        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }

    argv[argc] = NULL;
    return argc;
}

int main(void)
{
    /* Keep the shell running when Ctrl+C is pressed */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    {
        perror("myshell: signal");
        return 1;
    }

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
            break;
        }
        if (r == -1)
        {
            fprintf(stderr, "myshell: input too long (max %d characters)\n", MAX_INPUT);
            continue;
        }

        
        snprintf(copy, sizeof copy, "%s", line);
        normalize_operators(line, norm_line, sizeof norm_line);
        int argc = parse(norm_line, argv);

        if (argc < 0)
        {
            fprintf(stderr, "myshell: malformed command\n");
            continue;
        }

        if (argc > 0 &&
            (strchr("&;", argv[0][0]) != NULL ||
             validate_syntax(argc, argv) != 0))
        {
            fprintf(stderr, "myshell: malformed command\n");
            continue;
        }

        if (argc == 0)
            continue;

        add_to_history(copy);

        int pipe_idx = find_pipe(argc, argv);
        if (pipe_idx != -1)
            execute_pipeline(pipe_idx, argv);
        else
            execute_command(argc, argv);
    }
    return 0;
}
