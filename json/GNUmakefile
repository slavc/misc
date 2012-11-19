CXXFLAGS=-g -Wall -std=c++0x

all: json

json: json.o
	$(CXX) -o $@ $(CXXFLAGS) $<

clean:
	rm -f *.o json
