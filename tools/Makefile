.PHONY: install

CFLAGS=-O2 -g -Wall

all: findsrc grepsrc color hex

install: findsrc grepsrc indexsrc color hex
	install -d -m 700 "${HOME}/bin"
	install -m 700 findsrc "${HOME}/bin"
	install -m 700 grepsrc "${HOME}/bin"
	install -m 700 indexsrc "${HOME}/bin"
	install -m 700 uniqsrc "${HOME}/bin"
	install -m 700 color "${HOME}/bin"
	install -m 700 hex "${HOME}/bin"
	install -m 700 data2c.py "${HOME}/bin"
	ln -fs "${HOME}/bin/findsrc" "${HOME}/bin/findmak"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepisrc"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepsym"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepisym"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepmak"
	ln -fs "${HOME}/bin/grepsrc" "${HOME}/bin/grepimak"

uninstall:
	rm -f "${HOME}/bin/{findsrc,grepsrc,indexsrc,uniqsrc,findmak,grepisrc,grepsym,grepisym,grepmak,grepimak,color,hex,data2c.py}"

color: color.o
	${CC} ${CFLAGS} -o $@ color.o

hex: hex.o
	${CC} ${CFLAGS} -o $@ hex.o

clean:
	rm -f *.o color hex
