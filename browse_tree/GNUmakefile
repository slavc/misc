CC=gcc
CFLAGS+=-O2 -Wall -g $(shell pkg-config --cflags gtk+-2.0) -Iclib/include -Iinclude
LDFLAGS+=$(shell pkg-config --libs gtk+-2.0) -lz

all: bin/ bin/browse_tree

bin/:
	mkdir -p $@

bin/browse_tree: src/main.o src/tree.o src/descr.o clib/lib/libclib.a
	$(CC) -o $@ $(LDFLAGS) $^

clib/lib/libclib.a:
	$(MAKE) -C clib

clean:
	rm -rf src/*.o bin/
	$(MAKE) -C clib clean
