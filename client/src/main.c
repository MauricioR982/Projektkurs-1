#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include "game_data.h"
#include "hunter.h"
#include "obstacle.h"
#include "sprinter.h"


struct game {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;




    UDPsocket pSocket;
	IPaddress serverAddress;
	UDPpacket *pPacket;


};
typedef struct game Game;

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int main(int argv, char** args){


    return 0;
}

int initiate(Game *pGame) {

}

void run(Game *pGame) {

}

void close(Game *pGame) {
    
}