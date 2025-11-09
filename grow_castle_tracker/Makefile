CC = gcc
CFLAGS = -Iinclude -Wall
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = bin/grow_castle_tool

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(OBJ) -o $(BIN) $(CFLAGS)

clean:
	rm -rf src/*.o $(BIN)
