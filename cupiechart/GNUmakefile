CFLAGS+=-g $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS+=$(shell pkg-config --libs gtk+-2.0) -lm

all: custpiechart.o

clean:
	rm -f ./*.o test_custpiechart

test: test_custpiechart

test_custpiechart: custpiechart.o test_custpiechart.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
