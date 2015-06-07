CFLAGS = -std=c99 -Wall -O2

.PHONY: all clean

all: iobound

iobound: main.o
	$(CC) -lm -o $@ $<

clean:
	rm -f *.o iobound
