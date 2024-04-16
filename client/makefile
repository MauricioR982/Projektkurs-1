CC := gcc
CFLAGS := -I/mingw64/include/SDL2 -D_REENTRANT
LDFLAGS := -L/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_net

TARGET := client
SRC := udpc.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)
