CFLAGS = -Wall -ggdb -pedantic -std=c99
BIN=chip8
LIBS=-lSDL2
SRC=main.c chip8.c

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

all:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(BIN)

clean:
	rm $(BIN)
