CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
# Sanitizer build for the bounds tests (SR-002): any out-of-bounds access aborts the program
ASAN   = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

# The shell is built from these modules (NF-004). src/shell.c only includes them.
SRCS   = src/main.c src/input.c src/parser.c src/builtins.c src/history.c \
         src/redirect.c src/signals.c src/executor.c
HDRS   = $(wildcard src/*.h)

all: myshell simple_shell

myshell: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o myshell $(SRCS)

simple_shell: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o simple_shell $(SRCS)

unit_tests: tests/unit_tests.c src/shell.c $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o unit_tests tests/unit_tests.c

asan: tests/unit_tests.c $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(ASAN) -o myshell_asan $(SRCS)
	$(CC) $(CFLAGS) $(ASAN) -o unit_tests_asan tests/unit_tests.c

# Runs the unit tests, then every shell test script in tests/
test: all unit_tests
	-$(MAKE) --no-print-directory asan
	./unit_tests
	@for t in tests/*.sh; do echo "== $$t"; bash $$t || exit 1; done

clean:
	rm -f myshell simple_shell unit_tests myshell_asan unit_tests_asan
