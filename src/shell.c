#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

#define INPUT_SIZE 1024
#define MAX_ARGS 64

static int malformed(const char *s) {
    return strpbrk(s, "|;&<>") != NULL;
}

static char *find_executable(const char *cmd) {
    if (strchr(cmd, '/')) return strdup(cmd);

    const char *path = getenv("PATH");
    if (!path) return NULL;

    char *paths = strdup(path);
    if (!paths) return NULL;

    char *save = NULL;
    char *dir = strtok_r(paths, ":", &save);

    while (dir) {
        size_t n = strlen(dir) + strlen(cmd) + 2;
        char *candidate = malloc(n);
        if (!candidate) {
            free(paths);
            return NULL;
        }

        snprintf(candidate, n, "%s/%s", dir, cmd);
        if (access(candidate, X_OK) == 0) {
            free(paths);
            return candidate;
        }

        free(candidate);
        dir = strtok_r(NULL, ":", &save);
    }

    free(paths);
    return NULL;
}

int main(void) {
    char line[INPUT_SIZE];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        size_t len = strlen(line);
        if (len && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        } else {
            int ch = getchar();
            if (ch != '\n' && ch != EOF) {
                while (ch != '\n' && ch != EOF) ch = getchar();
                printf("Error: input line is too long.\n");
                continue;
            }
        }

        if (strspn(line, " \t\r") == strlen(line)) continue;
        if (malformed(line)) {
            printf("Error: unsupported shell operator.\n");
            continue;
        }

        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(line, " \t\r");

        while (token && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\r");
        }
        args[argc] = NULL;
        if (!argc) continue;

        if (strcmp(args[0], "cd") == 0) {
            const char *target = args[1] ? args[1] : getenv("HOME");

            if (!target) {
                fprintf(stderr, "myshell: HOME is not set\n");
            } else if (chdir(target) != 0) {
                perror("cd");
            }
            continue;
        }

        if (strcmp(args[0], "pwd") == 0) {
            char cwd[4096];

            if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
            else perror("pwd");
            continue;
        }

        if (strcmp(args[0], "help") == 0) {
            printf("myshell - available built-in commands:\n");
            printf("  cd [directory]  Change the current directory\n");
            printf("  pwd             Print the current directory\n");
            printf("  help            Display this help message\n");
            printf("External commands are searched for using PATH.\n");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            char *executable = find_executable(args[0]);

            if (!executable) {
                fprintf(stderr, "myshell: %s: command not found\n", args[0]);
                _exit(127);
            }

            execv(executable, args);
            perror(args[0]);
            _exit(126);
        } else {
            int status;
            while (waitpid(pid, &status, 0) == -1) {
                if (errno != EINTR) {
                    perror("waitpid");
                    break;
                }
            }
        }
    }

    return 0;
}
