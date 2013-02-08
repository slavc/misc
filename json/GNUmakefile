CXXFLAGS+=-Wall
CXXFLAGS+=-std=c++11
#CXXFLAGS+=-g
CXXFLAGS+=-O2
CXXFLAGS+=-pg

CFLAGS+=-Wall
#CFLAGS+=-g

LDFLAGS+=-pg


all: test

test: parse.o test.o json.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^

js0n: js0n.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

clean:
	rm -f *.o test js0n gmon.out
