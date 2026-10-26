#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define INPUT_SIZE 1024
#define MAX_ARGS 64

static int malformed(const char *line) {
    return strpbrk(line, "|;&<>") != NULL;
}

static char *find_executable(const char *cmd) {
    if (strchr(cmd, '/')) return strdup(cmd);

    const char *path = getenv("PATH");
    if (path == NULL) return NULL;

    char *paths = strdup(path);
    if (paths == NULL) return NULL;

    char *saveptr = NULL;
    char *dir = strtok_r(paths, ":", &saveptr);

    while (dir != NULL) {
        size_t size = strlen(dir) + strlen(cmd) + 2;
        char *candidate = malloc(size);
        if (candidate == NULL) {
            free(paths);
            return NULL;
        }

        snprintf(candidate, size, "%s/%s", dir, cmd);

        if (access(candidate, X_OK) == 0) {
            free(paths);
            return candidate;
        }

        free(candidate);
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(paths);
    return NULL;
}

int main(void) {
    char line[INPUT_SIZE];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
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

        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\r");
        }
        args[argc] = NULL;
        if (argc == 0) continue;

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            char *executable = find_executable(args[0]);
            if (executable != NULL) {
                execv(executable, args);
                free(executable);
            }
            _exit(127);
        } else {
            int status;
            while (waitpid(pid, &status, 0) == -1) {
                /* Retry if interrupted. */
            }
        }
    }

    return 0;
}
