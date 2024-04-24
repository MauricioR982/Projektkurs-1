#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL_net.h>
#include "udpserver.h"

Client clients[MAX_CLIENTS];

void initClients();

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
	if (!(sd = SDLNet_UDP_Open(2001)))
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
	
	initClients();

	/* Main loop */
	stop = 0;
	while (!stop) {
        if (SDLNet_UDP_Recv(sd, p)) {
            int clientIndex = -1;
            // Sök efter befintlig klient
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active &&
                    clients[i].address.host == p->address.host &&
                    clients[i].address.port == p->address.port) {
                    clientIndex = i;
                    break;
                }
            }
            // Om ingen befintlig klient hittades, leta efter en ledig plats
            if (clientIndex == -1) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].active) {
                        clients[i].active = 1;
                        clients[i].address = p->address;
						int index = i;
                        clients[i].id = index+1; // Använd arrayindex som ID
                        clientIndex = index;
                        printf("\nNew client, registered ID: %d\n", clients[i].id);
                        break;
                    }
                }
            }
            if (clientIndex != -1) {
                // Hantera paket från klient
                printf("\n*** UDP-packet incoming ***:\n\n");
				printf("    %-15s    %d\n", "Client ID:", clients[clientIndex].id); // Ändra formatsträng från %s till %d
				printf("    %-15s    %s\n", "Channel:", (p->channel == -1 ? "No specific" : "Specific channel"));
				printf("    %-15s    %s\n", "Data:", (char *)p->data);
				printf("    %-15s    %-5d\n", "Length:", p->len);
				printf("    %-15s    %-5d\n", "Maxlength:", p->maxlen);
				printf("    %-15s    %-5d\n", "Status:", p->status);
				printf("    %-15s    %s %u\n", "Sender address:", SDLNet_ResolveIP(&p->address), (p->address.port));

                // Stoppa servern om klient skickar "stop"
                if (strcmp((char *)p->data, "stop") == 0) {
                    stop = 1;
                }
            } else {
                printf("No available slots for new clients.\n");
            }
        }
    }
	SDLNet_FreePacket(p);
}

void initClients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
    }
} 