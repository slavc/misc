CC=gcc
CFLAGS=-Wall -g -DTEST_FILE

all: test_file

test_file: file.o xmem.o
	$(CC) -o $@ $(LDFLAGS) $^

clean:
	rm -f *.o test_file
