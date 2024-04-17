# Makefile for compiling the game

theGame: main.o sprinter.o hunter.o obstacle.o game_states.o
	gcc -o theGame main.o sprinter.o hunter.o obstacle.o game_states.o -L/opt/homebrew/lib/ -lSDL2 -lSDL2_image -lSDL2_net
main.o: main.c
	gcc -c main.c -I/opt/homebrew/include/SDL2
sprinter.o: sprinter.c
	gcc -c sprinter.c
hunter.o: hunter.c
	gcc -c hunter.c
obstacle.o: obstacle.c
	gcc -c obstacle.c	
game_states.o: game_states.c
	gcc -c game_states.c	
clean:
	rm *.o
	rm theGame