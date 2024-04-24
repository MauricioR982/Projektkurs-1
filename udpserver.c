#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL_net.h>
#include "udpserver.h"
 
void initiateServer(int argc, char **argv)
{
	UDPsocket sd;       /* Socket descriptor */
	UDPpacket *p;       /* Pointer to packet memory */
	int stop;
 
	/* Initialize SDL_net */
	if (SDLNet_Init() < 0)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}
	else
	{
    	printf("Host with name 'server' for network initialized successfully.\n");
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
	stop = 0;
	while (!stop)
	{
		/* Wait a packet. UDP_Recv returns != 0 if a packet is coming */
		if (SDLNet_UDP_Recv(sd, p))
		{
			printf("\n*** UDP-packet incoming ***:\n\n");
			printf("    %-15s    %-5d\n", "Channel:", p->channel);
			printf("    %-15s    %s\n", "Data:", (char *)p->data);
			printf("    %-15s    %-5d\n", "Length:", p->len);
			printf("    %-15s    %-5d\n", "Maxlength:", p->maxlen);
			printf("    %-15s    %-5d\n", "Status:", p->status);
			printf("    %-15s    %s %u\n", "Sender address:", SDLNet_ResolveIP(&p->address), p->address.port);
 
			/* Stop if packet contains "stop" */
			if (strcmp((char *)p->data, "stop") == 0)
				stop = 1;
		}		
	}
	SDLNet_FreePacket(p);
} 