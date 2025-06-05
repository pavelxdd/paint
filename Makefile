CC=gcc
CFLAGS=`pkg-config --cflags sdl2` -Wall -Wextra -std=gnu17
LDFLAGS=`pkg-config --libs sdl2`

all: main

main: main.o
	$(CC) -o $@ $^ $(LDFLAGS) -lm

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main.o main

.PHONY: all clean
