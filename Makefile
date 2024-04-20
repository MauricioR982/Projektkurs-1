# A simple Makefile for compiling SDL projects with networking

theGame: main.o sprinter.o hunter.o obstacle.o game_states.o network.o
	gcc -o theGame main.o sprinter.o hunter.o obstacle.o game_states.o network.o -L/opt/homebrew/lib/ -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_mixer

main.o: main.c
	gcc -c main.c -I/opt/homebrew/include/SDL2

sprinter.o: sprinter.c
	gcc -c sprinter.c -I/opt/homebrew/include/SDL2

hunter.o: hunter.c
	gcc -c hunter.c -I/opt/homebrew/include/SDL2

obstacle.o: obstacle.c
	gcc -c obstacle.c -I/opt/homebrew/include/SDL2

game_states.o: game_states.c
	gcc -c game_states.c -I/opt/homebrew/include/SDL2

network.o: network.c network.h
	gcc -c network.c -I/opt/homebrew/include/SDL2

clean:
	rm -f *.o theGame