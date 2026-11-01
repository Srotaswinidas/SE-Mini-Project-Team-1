/* redirect.h - '<' and '>' redirection (I/O Module). */
#ifndef REDIRECT_H
#define REDIRECT_H

int same_file(const char *a, const char *b);
const char *redirect_target(char **argv, const char *op);
int extract_redirection(char **argv, char **infile, char **outfile);
int apply_redirection(const char *infile, const char *outfile);

#endif
