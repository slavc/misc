.PHONY: install

CFLAGS=-O2 -g

all: findsrc grepsrc color hex

install: findsrc grepsrc indexsrc color hex
	install -d -m 700 "${HOME}/bin"
	install -m 700 findsrc "${HOME}/bin"
	install -m 700 grepsrc "${HOME}/bin"
	install -m 700 indexsrc "${HOME}/bin"
	install -m 700 color "${HOME}/bin"
	install -m 700 hex "${HOME}/bin"
	ln -fs "${HOME}/bin/findsrc" "${HOME}/bin/findmak"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepisrc"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepsym"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepisym"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepmak"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepimak"

uninstall:
	rm -f "${HOME}/bin/{findsrc,grepsrc,indexsrc,findmak,grepisrc,grepsym,grepisym,grepmak,grepimak,color}"

color: color.o
	${CC} ${CFLAGS} -o $@ color.o

hex: hex.o
	${CC} ${CFLAGS} -o $@ hex.o

clean:
	rm -f *.o
