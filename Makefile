CC=gcc
CFLAGS=`pkg-config --cflags sdl2` -Wall -Wextra -std=gnu17
LDFLAGS=`pkg-config --libs sdl2` -lm

SRCS=main.c draw.c palette.c
OBJS=$(SRCS:.c=.o)
TARGET=main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
