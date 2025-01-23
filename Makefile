CC = gcc
ZIG = zig cc
WIN_CC = x86_64-w64-mingw32-gcc

CFLAGS = -Wall -Wextra
IFLAGS = -Ilib/include

LIBS = -L lib
LIBS += -lSDL3 -lSDL3_ttf

WINLIBS = -Llib/windows
WINLIBS += -lSDL3_ttf -lSDL3.dll -l:freetype.lib

SRCS = src/*.c

BIN = j-ascii

all: linux

linux: $(SRCS)
	$(CC) $(CFLAGS) -o $(BIN) $^ $(IFLAGS) $(LIBS)

windows: $(SRCS)
	$(WIN_CC) $(CFLAGS) -o release/$(BIN) $^ $(IFLAGS) $(WINLIBS)





