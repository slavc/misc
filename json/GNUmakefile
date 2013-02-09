CXXFLAGS+=-Wall
CXXFLAGS+=-std=c++11
#CXXFLAGS+=-g
CXXFLAGS+=-O2
#CXXFLAGS+=-pg

CFLAGS+=-Wall
#CFLAGS+=-g

#LDFLAGS+=-pg
#LDFLAGS+=-g


all: test

test: parse.o test.o json.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

js0n: js0n.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o test js0n gmon.out

remake: clean all

gprof: test
	./test auctions.json
	gprof test gmon.out >> gprof.out
	less gprof.out
	
