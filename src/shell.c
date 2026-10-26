#include <stdio.h>
#include <string.h>

#define INPUT_SIZE 1024

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

        if (strspn(line, " \t\r") == strlen(line)) {
            continue;
        }

        printf("You entered: %s\n", line);
    }

    return 0;
}
