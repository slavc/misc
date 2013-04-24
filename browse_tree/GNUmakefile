CC=gcc
CFLAGS=-Wall -g $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS=$(shell pkg-config --libs gtk+-2.0)

all: browse_tree

browse_tree: main.o tree.o xmem.o
	$(CC) -o $@ $(LDFLAGS) $^

clean:
	rm -f *.o browse_tree
