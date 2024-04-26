# A simple Makefile for compiling SDL projects with networking on Windows using MinGW

# Define compiler and compiler flag variables
CC = gcc
# Include directory where SDL2 and related libraries headers are located
CFLAGS = -c -I"C:/SDL2/include"
# Library directory where SDL2 and related libraries binaries are located
LFLAGS = -L"C:/SDL2/lib" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_mixer

# Define your target executable
TARGET = theGame

# Define object files
OBJS = main.o sprinter.o hunter.o obstacle.o game_states.o udpclient.o udpserver.o

# Rule to link the object files into the executable
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LFLAGS)

# Rule to compile the main object file
main.o: main.c game_types.h
	$(CC) $(CFLAGS) main.c

# Rule to compile the sprinter object file
sprinter.o: sprinter.c game_types.h
	$(CC) $(CFLAGS) sprinter.c

# Rule to compile the hunter object file
hunter.o: hunter.c game_types.h
	$(CC) $(CFLAGS) hunter.c

# Rule to compile the obstacle object file
obstacle.o: obstacle.c game_types.h
	$(CC) $(CFLAGS) obstacle.c

# Rule to compile the game states object file
game_states.o: game_states.c game_types.h
	$(CC) $(CFLAGS) game_states.c

# Rule to clean up compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# Ensure that the 'all' target is the default.
all: $(TARGET)
