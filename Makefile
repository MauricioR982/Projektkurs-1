# A simple Makefile for compiling small SDL projects

theGame: main.o sprinter.o Alien.o world.o
	gcc -o theGame main.o sprinter.o world.o -L/opt/homebrew/lib/ -lSDL2 -lSDL2_image
main.o: main.c
	gcc -c main.c -I/opt/homebrew/include/SDL2
sprinter.o: sprinter.c
	gcc -c sprinter.c
world.o: world.c
	gcc -c world.c
clean:
	rm *.o
	rm theGame