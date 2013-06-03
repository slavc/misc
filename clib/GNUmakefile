CC=cc

CFLAGS+=-Iinclude
CFLAGS+=-Wall
CFLAGS+=-O2
CFLAGS+=-std=c99

all: lib/ lib/libclib.a

lib/:
	mkdir -p $@

lib/libclib.a: src/str.o src/file.o src/xmem.o
	ar -qc $@ $^

test: lib/ test/test
	test/test

test/test: test/test.o lib/libclib.a
	$(CC) -o $@ $^

doc: doc/ doc/html/

doc/:
	mkdir -p $@

doc/html/: doxy.cfg
	doxygen doxy.cfg

clean:
	rm -rf lib/ src/*.o test/*.o test/test doc/
