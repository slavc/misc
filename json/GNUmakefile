CXX=clang++
CC=clang
#CXX=g++
#CC=gcc

CXXFLAGS+=-Wall
CXXFLAGS+=-std=c++11
#CXXFLAGS+=-g
CXXFLAGS+=-O2
#CXXFLAGS+=-pg

CFLAGS+=-Wall
#CFLAGS+=-g
#CFLAGS+=-pg

#LDFLAGS+=-pg
#LDFLAGS+=-g


all: test

test: parse.o test.o json.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

test2: parse.o test2.o json.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o test test2 js0n gmon.out

remake: clean all

gprof: test
	./test big.json
	gprof test gmon.out >> gprof.out
	less gprof.out
	
