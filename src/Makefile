CFLAGS = -Wall -ggdb -pedantic -std=c99
BIN=chip8
SRC=main.c chip8.c

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

all:
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	rm $(BIN)
