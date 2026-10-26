
#include <stdio.h>
#include <string.h>

int main(void) {
    char line[1024];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';
        printf("You entered: %s\n", line);
    }

    return 0;
}
