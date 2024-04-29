#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
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
	UDPpacket *pPacket;
    //IPaddress clients[MAX_ROCKETS]; ändra max rockets till antal spelare
    int nrOfClients;
    ServerData sData;

};
typedef struct game Game;

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int main(int argv, char** args){
    Game g={0};
    if(!initiate(&g)) return 1;
    run(&g);
    close(&g);

    return 0;
}

int initiate(Game *pGame) {

}

void run(Game *pGame) {

}

void close(Game *pGame) {
    
}