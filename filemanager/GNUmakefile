CFLAGS=$(shell pkg-config --cflags gtk+-2.0) -g
LDFLAGS=$(shell pkg-config --libs gtk+-2.0)

BIN=filemanager
OBJ=main.o fm.o util.o

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) $(OBJ)
