CFLAGS += -g
CC = cc

all: lisp

lisp: src/lisp.o src/parse.o src/eval.o src/mem.o src/util.o
	$(CC) -o $@ src/lisp.o src/parse.o src/eval.o src/mem.o src/util.o

clean:
	rm -rf lisp src/*.o
