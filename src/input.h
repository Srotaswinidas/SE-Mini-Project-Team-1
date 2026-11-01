/* input.h - reading a command line (Input Module). */
#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

int read_line(char *buf, size_t size);
void normalize_operators(const char *src, char *dst, size_t dst_size);

#endif
