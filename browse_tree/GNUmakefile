CC=gcc
CFLAGS+=-O2 -Wall -g $(shell pkg-config --cflags gtk+-2.0) -Iclib/include
LDFLAGS+=$(shell pkg-config --libs gtk+-2.0)

all: browse_tree

browse_tree: main.o tree.o descr.o clib/lib/libclib.a
	$(CC) -o $@ $(LDFLAGS) $^

clean:
	rm -f *.o browse_tree
