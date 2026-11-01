/* history.h - the list of earlier commands (History Module). */
#ifndef HISTORY_H
#define HISTORY_H

#include "shell.h"

extern char *history[MAX_HISTORY];   /* oldest first */
extern int hist_count;               /* number of entries in use */

void add_to_history(const char *line);
void cleanup_history(void);

#endif
