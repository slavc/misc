all: libcxxcgi cxxcgipp

clean:
	rm -f *.o libcxxcgi{.so.1.0,.so.1,.so} cxxcgipp

cxxcgipp:
	$(CXX) -o cxxcgipp cxxcgipp.cc

libcxxcgi:
	gcc -I/usr/local/include -L/usr/local/lib -lsqlite3 -shared -fpic -o libcxxcgi.so.1.0 libcxxcgi.cc
	ln -fs libcxxcgi.so.1.0 libcxxcgi.so.1
	ln -fs libcxxcgi.so.1 libcxxcgi.so

