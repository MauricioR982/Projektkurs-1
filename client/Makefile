SRCDIR=./src
COMMONSRCDIR=../lib/src
INCDIR=../lib/include
CC=gcc
INCLUDE = -IC:/msys64/mingw64/include/SDL2 -I$(INCDIR) 
CFLAGS = -g $(INCLUDE) -c
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_net -mwindows -lm

gameClient: main.o sprinter.o hunter.o obstacle.o game_data.o text.o game_timer.o
	$(CC) -o gameClient main.o sprinter.o hunter.o obstacle.o game_data.o text.o game_timer.o $(LDFLAGS)

main.o: $(SRCDIR)/main.c $(INCDIR)/sprinter.h $(INCDIR)/hunter.h $(INCDIR)/obstacle.h $(INCDIR)/game_data.h $(INCDIR)/text.h
	$(CC) $(CFLAGS) $(SRCDIR)/main.c

sprinter.o: $(COMMONSRCDIR)/sprinter.c $(INCDIR)/sprinter.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/sprinter.c

hunter.o: $(COMMONSRCDIR)/hunter.c $(INCDIR)/hunter.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/hunter.c

obstacle.o: $(COMMONSRCDIR)/obstacle.c $(INCDIR)/obstacle.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/obstacle.c

text.o: $(COMMONSRCDIR)/text.c $(INCDIR)/text.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/text.c

game_timer.o: $(COMMONSRCDIR)/game_timer.c $(INCDIR)/game_timer.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/game_timer.c