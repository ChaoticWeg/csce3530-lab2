CC=gcc
CFLAGS=-Werror -Wall

SIDE_NAMES=server client

all: $(SIDE_NAMES)

shared_binaries := $(patsubst src/%.c,bin/%.tmp.o,$(wildcard src/*.c))
$(shared_binaries): bin/%.tmp.o: src/*.c
	@mkdir -p bin
	$(CC) $(CLAGS) -c -o bin/$*.tmp.o src/$*.c

.SECONDEXPANSION:
$(SIDE_NAMES): %: $$*.c $(shared_binaries)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean fresh

clean:
	@rm -f $(SIDE_NAMES) **/*.o >/dev/null 2>&1
	@rm -rf bin/

fresh: clean all
