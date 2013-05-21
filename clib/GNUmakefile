CC=gcc
CFLAGS+=-Wall -g -DTEST_FP_GETS -DTEST_STR

all: test_file

test_file: file.o xmem.o
	$(CC) -o $@ $(LDFLAGS) $^

test_str: str.o xmem.o
	$(CC) -o $@ $(LDFLAGS) $^

clean:
	rm -f *.o test_file
