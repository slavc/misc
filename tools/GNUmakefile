all: findsrc grepsrc

install: findsrc grepsrc indexsrc
	mkdir -p ~/bin
	cp -f findsrc grepsrc indexsrc ~/bin
	ln -fs ~/bin/findsrc ~/bin/findmak
	ln -fs ~/bin/grepsrc ~/bin/grepisrc
	ln -fs ~/bin/grepsrc ~/bin/grepsym
	ln -fs ~/bin/grepsrc ~/bin/grepisym
	ln -fs ~/bin/grepsrc ~/bin/grepmak
	ln -fs ~/bin/grepsrc ~/bin/grepimak
