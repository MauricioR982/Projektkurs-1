# A simple Makefile for compiling SDL projects with networking on Windows using MinGW

# Define compiler and compiler flag variables
CC = gcc
CFLAGS = -c -I C:/SDL2/include/SDL2
LFLAGS = -L C:/SDL2/lib -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_mixer

# Define your target executable
TARGET = theGame.exe

# Define object files
OBJS = main.o sprinter.o hunter.o obstacle.o game_states.o

# Compile the main executable
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LFLAGS)

# Compile the main object file
main.o: main.c game_types.h
	$(CC) $(CFLAGS) main.c

# Compile the sprinter object file
sprinter.o: sprinter.c game_types.h
	$(CC) $(CFLAGS) sprinter.c

# Compile the hunter object file
hunter.o: hunter.c game_types.h
	$(CC) $(CFLAGS) hunter.c

# Compile the obstacle object file
obstacle.o: obstacle.c game_types.h
	$(CC) $(CFLAGS) obstacle.c

# Compile the game states object file
game_states.o: game_states.c game_types.h
	$(CC) $(CFLAGS) game_states.c

# Clean up compiled files
clean:
	del $(OBJS) $(TARGET)