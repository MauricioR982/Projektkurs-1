#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include "game_states.h"
#include "hunter.h"
#include "obstacle.h"
#include "sprinter.h"


struct game {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;




    UDPsocket pSocket;
	IPaddress serverAddress;
	UDPpacket *pPacket;


}