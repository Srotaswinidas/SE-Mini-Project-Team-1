/* history.c - History Module (F-013, NF-002, SR-005).
 * Keeps copies of the newest MAX_HISTORY commands and frees them on exit. */
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "history.h"

char *history[MAX_HISTORY];
int hist_count = 0;

/* Release all command history allocations. */
void cleanup_history(void)
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
void add_to_history(const char *line)
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
