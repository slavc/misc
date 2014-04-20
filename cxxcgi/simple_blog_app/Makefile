CFLAGS=-L.. -I.. -L/usr/local/lib -I/usr/local/include -g
CXXFLAGS=$(CFLAGS)

all: blog.cgi

clean:
	rm -f blog.cgi blog.cc

blog.cgi: blog.cc
	$(CXX) $(CXXFLAGS) -o blog.cgi blog.cc -lcccgi -lsqlite3

blog.cc:
	../cccgipp -i blog.cccgi
