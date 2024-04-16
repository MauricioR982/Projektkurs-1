#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "udps.h"
#include <SDL2/SDL_net.h>

void handleIncomingData(UDPsocket sd);
 
void initServer() {
    UDPsocket sd;

    if (SDLNet_Init() < 0) {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    if (!(sd = SDLNet_UDP_Open(2000))) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    // Add more logic to handle incoming data
    handleIncomingData(sd);
}
void handleIncomingData(UDPsocket sd) {
    UDPpacket *p = SDLNet_AllocPacket(512);
    if (!p) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        return;
    }
    
    if (SDLNet_UDP_Recv(sd, p)) {
        printf("Received: %s from %x %x\n", (char *)p->data, p->address.host, p->address.port);
        // Process here
    }
    SDLNet_FreePacket(p);
}
