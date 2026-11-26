CFLAGS := -std=c99 -Wall -Wextra -g -fsanitize=address,undefined

google: src/*.c src/*.h
	cc $(CFLAGS) src/*.c -o google

run: google
	./google

clean:
	rm -rf google google.dSYM
