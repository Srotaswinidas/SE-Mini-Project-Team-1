#include <stdio.h>
#include <string.h>

#define INPUT_SIZE 1024
#define MAX_ARGS 64

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
                while (ch != '\n' && ch != EOF) {
                    ch = getchar();
                }

                printf("Error: input line is too long.\n");
                continue;
            }
        }

        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(line, " \t\r");

        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\r");
        }

        args[argc] = NULL;

        if (argc == 0) {
            continue;
        }

        printf("Command: %s\n", args[0]);

        for (int i = 1; i < argc; i++) {
            printf("Argument %d: %s\n", i, args[i]);
        }
    }

    return 0;
}
