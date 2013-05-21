CC=gcc
CFLAGS+=-Wall -g $(shell pkg-config --cflags gtk+-2.0) -I../clib/
LDFLAGS+=$(shell pkg-config --libs gtk+-2.0)

all: browse_tree

browse_tree: main.o tree.o descr.o ../clib/xmem.o ../clib/file.o ../clib/str.o
	$(CC) -o $@ $(LDFLAGS) $^

descr_unittest: ../clib/xmem.o
	$(CC) -DDESCR_UNITTEST $(CFLAGS) $(LDFLAGS) -o $@ $^ descr.c
	./descr_unittest ; rm descr_unittest

clean:
	rm -f *.o browse_tree
