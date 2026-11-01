/* parser.h - splitting and checking a command line (Parser Module). */
#ifndef PARSER_H
#define PARSER_H

int parse(char *line, char **argv);
const char *syntax_problem(int argc, char **argv);
int find_pipe(int argc, char **argv);

#endif
