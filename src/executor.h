/* executor.h - running commands (Execution Module). */
#ifndef EXECUTOR_H
#define EXECUTOR_H

void execute_command(int argc, char **argv);
void execute_pipeline(int pipe_idx, char **argv);

#endif
