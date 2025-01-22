CC = gcc
CFLAGS = -Wall -Wextra
IFLAGS = -Ilib/include

LFLAGS = -L lib
LFLAGS += -lSDL3 -lSDL3_ttf

SRCS = src/*.c

BIN = j-ascii

all: linux

linux: $(SRCS)
	$(CC) $(CFLAGS) -o $(BIN) $^ $(IFLAGS) $(LFLAGS)





