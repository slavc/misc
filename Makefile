.PHONY: all clean

all: vkhwinfo

clean:
	rm -f vkhwinfo

vkhwinfo: vkhwinfo.cpp
	g++ -o vkhwinfo vkhwinfo.cpp -lvulkan


