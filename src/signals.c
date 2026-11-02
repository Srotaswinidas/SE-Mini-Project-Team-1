/* signals.c - Signal Module (F-017).
 * The shell itself ignores Ctrl+C and Ctrl+\ so it stays usable. Child processes
 * set both back to the default (see executor.c), so Ctrl+C still stops a running command. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include "signals.h"

/* F-017: make the shell ignore Ctrl+C and Ctrl+\. Returns 0, or -1 after printing an error. */
int setup_signals(void)
{
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    {
        perror("myshell: signal");
        return -1;
    }
    signal(SIGQUIT, SIG_IGN);
    return 0;
}
