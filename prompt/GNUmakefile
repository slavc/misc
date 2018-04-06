PREFIX = /usr/local

CFLAGS += -Wall -Wno-switch -std=c99 -D_XOPEN_SOURCE=600

.PHONY: all install uninstall clean

all: bin/ bin/prompt

install: all
	install -d $(PREFIX)/bin
	install bin/prompt $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/prompt

bin/:
	mkdir bin

bin/prompt: src/main.o
	$(CC) -o $@ $<

clean:
	rm -rf bin/ src/*.o
