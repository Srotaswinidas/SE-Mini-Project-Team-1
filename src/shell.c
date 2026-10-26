#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wapy shell\it.h>

#define MAX_INPUT   1024
#define MAX_ARGS    64
#define MAX_HISTORY 100

static char *history[MAX_HISTORY];
static int hist_count = 0;

/* F-013: remember commands */
static void add_to_history(const char *line) {
    if (hist_count < MAX_HISTORY) {
        char *copy = strdup(line);
        if (copy != NULL) history[hist_count++] = copy;
    }
}

/* F-002 + SR-001: read a line. Returns 1 = ok, 0 = EOF (Ctrl+D), -1 = too long */
static int read_line(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        return 1;
    }
    if (feof(stdin)) return 1;            /* last line without newline */
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }   /* throw away the rest */
    return -1;
}

/* F-004: split the line into words. Returns word count, or -1 if malformed */
static int parse(char *line, char **argv) {
    int argc = 0;
    char *tok = strtok(line, " \t");
    while (tok != NULL) {
        if (argc >= MAX_ARGS - 1) return -1;     /* too many arguments */
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    return argc;
}

/* F-009 to F-013: built-in commands. Returns 1 if the command was a built-in */
static int run_builtin(int argc, char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        for (int i = 0; i < hist_count; i++) free(history[i]);
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = (argc > 1) ? argv[1] : getenv("HOME");
        if (dir == NULL || chdir(dir) != 0) perror("myshell: cd");
        return 1;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char cwd[4096];
        if (getcwd(cwd, sizeof cwd) != NULL) printf("%s\n", cwd);
        else perror("myshell: pwd");
        return 1;
    }
    if (strcmp(argv[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd [dir]   change directory\n");
        printf("  pwd        print current directory\n");
        printf("  help       show this help\n");
        printf("  history    show earlier commands\n");
        printf("  exit       leave the shell\n");
        printf("Any other command is run from PATH, e.g. ls -l\n");
        return 1;
    }
    if (strcmp(argv[0], "history") == 0) {
        for (int i = 0; i < hist_count; i++) printf("%4d  %s\n", i + 1, history[i]);
        return 1;
    }
    return 0;
}

/* F-005 to F-008: run an external command and wait for it */
static void run_external(char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("myshell: fork");
        return;
    }
    if (pid == 0) {                         /* child */
        execvp(argv[0], argv);              /* execvp searches PATH (F-006) */
        if (errno == ENOENT) fprintf(stderr, "myshell: %s: command not found\n", argv[0]);
        else perror("myshell");
        _exit(127);
    }
    int status;                             /* parent: F-007 wait for the child */
    waitpid(pid, &status, 0);
}

int main(void) {
    char line[MAX_INPUT + 2];
    char copy[MAX_INPUT + 2];
    char *argv[MAX_ARGS];

    while (1) {
        printf("myshell> ");                /* F-001 */
        fflush(stdout);

        int r = read_line(line, sizeof line);
        if (r == 0) { printf("\n"); break; }
        if (r == -1) {
            fprintf(stderr, "myshell: input too long (max %d characters)\n", MAX_INPUT);
            continue;
        }

        snprintf(copy, sizeof copy, "%s", line);
        int argc = parse(line, argv);

        if (argc < 0 || (argc > 0 && strchr("|<>&;", argv[0][0]) != NULL)) {
            fprintf(stderr, "myshell: malformed command\n");   /* SR-003 */
            continue;
        }
        if (argc == 0) continue;                               /* F-003 */

        add_to_history(copy);
        if (run_builtin(argc, argv)) continue;
        run_external(argv);
    }
    return 0;
}
