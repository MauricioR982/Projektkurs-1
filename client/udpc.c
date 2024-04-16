#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "udpc.h"
#include <SDL2/SDL_net.h>

void sendPlayerData(UDPsocket sd, IPaddress srvadd, const char* data);
 
void initClient(const char* host, int port) {
    UDPsocket sd;
    IPaddress srvadd;

    if (SDLNet_Init() < 0) {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    if (!(sd = SDLNet_UDP_Open(0))) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    if (SDLNet_ResolveHost(&srvadd, host, port) == -1) {
        fprintf(stderr, "SDLNet_ResolveHost(%s %d): %s\n", host, port, SDLNet_GetError());
        exit(EXIT_FAILURE);
    }
    // Example sending a message
    sendPlayerData(sd, srvadd, "Hello, server!");
}
void sendPlayerData(UDPsocket sd, IPaddress srvadd, const char* data) {
    UDPpacket *p = SDLNet_AllocPacket(512);
    if (!p) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        return;
    }
    strcpy((char *)p->data, data);
    p->address = srvadd;
    p->len = strlen((char *)p->data) + 1;
    SDLNet_UDP_Send(sd, -1, p);  // send packet
    SDLNet_FreePacket(p);
}

