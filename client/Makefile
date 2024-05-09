SRCDIR=./src
COMMONSRCDIR=../lib/src
INCDIR=../lib/include
CC=gcc
INCLUDE = -IC:/msys64/mingw64/include/SDL2 -I$(INCDIR) 
CFLAGS = -g $(INCLUDE) -c
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_net -mwindows -lm

gameClient: main.o sprinter.o hunter.o obstacle.o game_data.o text.o perk.o
	$(CC) -o gameClient main.o sprinter.o hunter.o obstacle.o game_data.o text.o perk.o $(LDFLAGS)

main.o: $(SRCDIR)/main.c $(INCDIR)/game_data.h
	$(CC) $(CFLAGS) $(SRCDIR)/main.c

sprinter.o: $(COMMONSRCDIR)/sprinter.c $(INCDIR)/sprinter.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/sprinter.c

hunter.o: $(COMMONSRCDIR)/hunter.c $(INCDIR)/hunter.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/hunter.c

obstacle.o: $(COMMONSRCDIR)/obstacle.c $(INCDIR)/obstacle.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/obstacle.c

text.o: $(COMMONSRCDIR)/text.c $(INCDIR)/text.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/text.c

perk.o: $(COMMONSRCDIR)/perk.c $(INCDIR)/perk.h
	$(CC) $(CFLAGS) $(COMMONSRCDIR)/perk.c