CC=gcc
CFLAGS=`pkg-config --cflags sdl2` -Wall -Wextra -std=c99
LDFLAGS=`pkg-config --libs sdl2`

all: main

main: main.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main.o main

.PHONY: all clean
