/* input.c - Input Module (F-002, SR-001, SR-002).
 * Reads one line without ever writing past the buffer, and puts spaces around
 * the operators | < > so the parser can split them like ordinary words. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include "shell.h"
#include "input.h"

/* F-002 + SR-001 + SR-002: read one line into buf (capacity 'size').
 * Never writes more than MAX_INPUT characters plus the terminating NUL.
 * Returns 1 = ok, 0 = EOF (Ctrl+D), -1 = too long, -2 = interrupted by a signal,
 *        -3 = the line contained a NUL byte */
int read_line(char *buf, size_t size)
{
    size_t limit = (size > 0) ? size - 1 : 0; /* room for the final NUL */
    if (limit > MAX_INPUT)
        limit = MAX_INPUT;
    size_t len = 0;
    int too_long = 0, bad_char = 0, c;

    errno = 0;
    while ((c = getchar()) != EOF && c != '\n')
    {
        if (c == '\0')
            bad_char = 1;
        else if (len < limit)
            buf[len++] = (char)c;
        else
            too_long = 1; /* keep reading, but store nothing more */
    }
    if (c == EOF)
    {
        if (ferror(stdin) && errno == EINTR)
        {
            clearerr(stdin);
            return -2;
        }
        if (len == 0 && !too_long && !bad_char)
            return 0; /* real end of input */
    }
    if (size > 0)
        buf[len] = '\0';
    if (bad_char)
        return -3;
    if (too_long)
        return -1;
    return 1;
}

/* Helper to pad spaces around |, <, and > so strtok can tokenize them cleanly */
void normalize_operators(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 2 < dst_size; i++)
    {
        if (src[i] == '|' || src[i] == '<' || src[i] == '>')
        {
            if (j > 0 && dst[j - 1] != ' ' && dst[j - 1] != '\t')
                dst[j++] = ' ';
            dst[j++] = src[i];
            if (src[i + 1] != ' ' && src[i + 1] != '\t' && src[i + 1] != '\0')
                dst[j++] = ' ';
        }
        else
        {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}
