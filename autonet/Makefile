CFLAGS=-Wall -std=c99 -O2

.PHONY: all clean

all: libexec/autonet

clean:
	rm -f src/*.o
	rm -rf libexec

libexec/autonet: src/autonet.o
	mkdir -p libexec
	${CC} -o libexec/autonet src/autonet.o

src/autonet.o: src/autonet.c
	${CC} ${CFLAGS} -c -o src/autonet.o src/autonet.c

install: all
	cp -R autonet /etc
	cp rc.d/autonet /etc/rc.d
	mkdir -p /usr/local/libexec
	cp libexec/autonet /usr/local/libexec
	grep pkg_scripts /etc/rc.conf.local || echo 'pkg_scripts=' >> /etc/rc.conf.local
	{ grep pkg_scripts /etc/rc.conf.local | grep autonet; } || sed -i 's/pkg_scripts=/pkg_scripts=autonet /g' /etc/rc.conf.local
