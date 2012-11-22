CXXFLAGS=-g -Wall -std=c++11

all: json parse

json: json.o
	$(CXX) -o $@ $(CXXFLAGS) $<

parse: parse.o
	$(CXX) -o $@ $(CXXFLAGS) $<

clean:
	rm -f *.o json parse
