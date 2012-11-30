CXXFLAGS=-g -Wall -std=c++11

all: test

test: parse.o test.o json.o
	$(CXX) -o $@ $(CXXFLAGS) $^

clean:
	rm -f *.o test
