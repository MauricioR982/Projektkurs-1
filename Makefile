# A simple Makefile for compiling small SDL projects

theGame: main.o sprinter.o hunter.o
	gcc -o theGame main.o sprinter.o hunter.o -L/opt/homebrew/lib/ -lSDL2 -lSDL2_image
main.o: main.c
	gcc -c main.c -I/opt/homebrew/include/SDL2
sprinter.o: sprinter.c
	gcc -c sprinter.c
world.o: world.c
	gcc -c world.c
hunter.o: hunter.c
	gcc -c hunter.c
clean:
	rm *.o
	rm theGame