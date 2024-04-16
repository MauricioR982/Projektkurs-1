#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <SDL2/SDL_net.h>

void handleIncomingData(UDPsocket sd);
 
int main(int argc, char **argv)
{
	UDPsocket sd;       /* Socket descriptor */
	UDPpacket *p;       /* Pointer to packet memory */
	int quit;
 
	/* Initialize SDL_net */
	if (SDLNet_Init() < 0)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}
 
	/* Open a socket */
	if (!(sd = SDLNet_UDP_Open(2000)))
	{
		fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}
 
	/* Make space for the packet */
	if (!(p = SDLNet_AllocPacket(512)))
	{
		fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}
 
	/* Main loop */
	quit = 0;
	while (!quit)
	{
		/* Wait a packet. UDP_Recv returns != 0 if a packet is coming */
		if (SDLNet_UDP_Recv(sd, p))
		{
			printf("UDP Packet incoming\n");
			printf("\tChan:    %d\n", p->channel);
			printf("\tData:    %s\n", (char *)p->data);
			printf("\tLen:     %d\n", p->len);
			printf("\tMaxlen:  %d\n", p->maxlen);
			printf("\tStatus:  %d\n", p->status);
			printf("\tAddress: %x %x\n", p->address.host, p->address.port);
 
			/* Quit if packet contains "quit" */
			if (strcmp((char *)p->data, "quit") == 0)
				quit = 1;
		}		
	}
 
	/* Clean and exit */
	SDLNet_FreePacket(p);
	SDLNet_Quit();
	return EXIT_SUCCESS;
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
