# Define compiler and compiler flag variables
CC = gcc
CFLAGS = -c -I C:/SDL2/include/SDL2
LFLAGS = -L C:/SDL2/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_net

# Define your target executable
TARGET = client.exe

# Define object files
OBJS = client.o

# Compile the main executable
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LFLAGS)

# Compile the client object file
client.o: client.c
	$(CC) $(CFLAGS) -c client.c -o client.o

# Clean up compiled files
clean:
	del $(OBJS) $(TARGET)
