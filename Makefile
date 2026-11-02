CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
ASAN = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

SRCS = src/main.c src/input.c src/parser.c src/builtins.c src/history.c \
       src/redirect.c src/signals.c src/executor.c
HDRS = $(wildcard src/*.h)

all: myshell simple_shell

myshell: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o myshell $(SRCS)

simple_shell: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o simple_shell $(SRCS)

asan: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(ASAN) -o myshell_asan $(SRCS)

test: all
	bash run_all_tests.sh

clean:
	rm -f myshell simple_shell myshell_asan unit_tests unit_tests_asan

.PHONY: all asan test clean
