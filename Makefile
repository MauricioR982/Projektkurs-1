# A simple Makefile for compiling small SDL projects

theGame: main.o sprinter.o hunter.o obstacle.o
	gcc -o theGame main.o sprinter.o hunter.o obstacle.o -L/opt/homebrew/lib/ -lSDL2 -lSDL2_image
main.o: main.c
	gcc -c main.c -I/opt/homebrew/include/SDL2
sprinter.o: sprinter.c
	gcc -c sprinter.c
hunter.o: hunter.c
	gcc -c hunter.c
obstacle.o: obstacle.c
	gcc -c obstacle.c	
clean:
	rm *.o
	rm theGame