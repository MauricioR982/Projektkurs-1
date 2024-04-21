//
// network.c
// Developed by Grupp 10 - Datateknik, project-start at 2024-03
//

#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static UDPsocket sd; // Socket descriptor
static IPaddress srvadd; // Server address
static bool isServer;
static UDPpacket *packet;
static ClientInfo clients[MAX_CLIENTS];

int network_init(char* host, Uint16 port, bool serverMode) {
    isServer = serverMode;
    memset(clients, 0, sizeof(clients));  // Initialize client-information
    
    if (SDLNet_Init() < 0) {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        return -1;
    }

    packet = SDLNet_AllocPacket(512);
    if (!packet) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        return -1;
    }

    if (isServer) {
        // Open a socket on a specific port
        sd = SDLNet_UDP_Open(port);
        if (!sd) {
            fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        printf("Server listening on port %d\n", port);
    } else {
        // Resolve server name and open a socket on random port
        if (SDLNet_ResolveHost(&srvadd, host, port) == -1) {
            fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        sd = SDLNet_UDP_Open(0);
        if (!sd) {
            fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        printf("Client trying to connect to %s:%d\n", host, port);
    }
    return 0;
}

void network_check_activity() {
    SDLNet_SocketSet set = SDLNet_AllocSocketSet(1);
    if (!set) {
        fprintf(stderr, "SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
        return;
    }
    if (SDLNet_UDP_AddSocket(set, sd) == -1) {
        fprintf(stderr, "SDLNet_AddSocket: %s\n", SDLNet_GetError());
        SDLNet_FreeSocketSet(set);
        return;
    }

    int numready = SDLNet_CheckSockets(set, 0); // Gör detta icke-blockerande
    if (numready > 0) {
        if (SDLNet_SocketReady(sd)) {
            if (isServer) {
                network_handle_server();
            } else {
                network_handle_client();
            }
        }
    }
    SDLNet_FreeSocketSet(set);
}

void network_handle_server() {
    int quit = 0;
    while(!quit)
    {
    if (SDLNet_UDP_Recv(sd, packet)) {
        printf("UDP Packet incoming\n");
        printf("\tChan:    %d\n", packet->channel);
        printf("\tData:    %s\n", (char *)packet->data);
        printf("\tLen:     %d\n", packet->len);
        printf("\tMaxlen:  %d\n", packet->maxlen);
        printf("\tStatus:  %d\n", packet->status);
        printf("\tAddress: %x %x\n", packet->address.host, packet->address.port);

        // Echo packet to all clients except sender
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].connected) {
                packet->address = clients[i].address;
                SDLNet_UDP_Send(sd, -1, packet);
            }
        }

        // Check if the packet is a quit command
        if (strcmp((char *)packet->data, "quit") == 0) {
            quit = 1;
            exit(0);
        }
    }
}
    network_cleanup();
}

void network_handle_client() {
    if (SDLNet_UDP_Recv(sd, packet)) {
        printf("Received from server: %s\n", (char *)packet->data);
    }
}

int find_or_add_client(IPaddress newClientAddr) {
    int emptySpot = -1;
    int countConnected = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].connected) {
            if (clients[i].address.host == newClientAddr.host && clients[i].address.port == newClientAddr.port) {
                // Client is already connected
                return i;
            }
            countConnected++;
        } else if (emptySpot == -1) {
            emptySpot = i;  // Save the first empty spot
        }
    }
    if (countConnected >= MAX_CLIENTS) {
        // Max clients reached, return -1
        return -1;
    }
    // Add new client
    if (emptySpot != -1) {
        clients[emptySpot].connected = true;
        clients[emptySpot].address = newClientAddr;
        return emptySpot;
    }
    return -1;
}


void network_cleanup() {
    SDLNet_FreePacket(packet);
    SDLNet_UDP_Close(sd);
    SDLNet_Quit();
}
