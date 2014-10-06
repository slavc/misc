CFLAGS = -std=c99 -Wall -O2

.PHONY: all clean

all: psprio

psprio: main.o procstat.o
	$(CC) -lm -o $@ $^

clean:
	rm -f *.o psprio
