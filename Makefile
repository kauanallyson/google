CC     := cc
CFLAGS := -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -Iinclude
SOURCES := $(wildcard src/*.c src/*/*.c)
HEADERS := $(wildcard include/*.h)

google: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o google

run: google
	./google

clean:
	rm -rf google google.dSYM
