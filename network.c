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

void network_check_activity(Uint32 timeout) {

    // Setup the socket set
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

    // Check for activity on the socket
    int numready = SDLNet_CheckSockets(set, timeout);
    if (numready == -1) {
        fprintf(stderr, "SDLNet_CheckSockets: %s\n", SDLNet_GetError());
    } else if (numready == 0) {
        // No activity
    } else {
        // If there is activity on our socket, process it
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
    if (SDLNet_SocketReady(sd)) {
        if (SDLNet_UDP_Recv(sd, packet)) {
            int clientIndex, x, y;
            // Förvänta att paketdata är formaterad som "clientIndex,x,y"
            if (sscanf((char *)packet->data, "%d,%d,%d", &clientIndex, &x, &y) == 3) {
                // Hitta eller lägg till klient, om inte fullt
                clientIndex = find_or_add_client(packet->address);
                if (clientIndex == -1) {
                    // Server full, skicka tillbaka avslagsmeddelande
                    const char* rejectMsg = "Server Full: No more connections allowed.";
                    memcpy(packet->data, rejectMsg, strlen(rejectMsg) + 1);
                    packet->len = strlen(rejectMsg) + 1;
                    SDLNet_UDP_Send(sd, -1, packet);
                } else {
                    // Uppdatera klientens position
                    update_player_position(clientIndex, x, y);

                    // Skicka uppdateringen till alla andra anslutna klienter
                    sprintf((char *)packet->data, "%d,%d,%d", clientIndex, x, y);
                    packet->len = strlen((char *)packet->data) + 1;
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].connected && i != clientIndex) {
                            packet->address = clients[i].address;
                            SDLNet_UDP_Send(sd, -1, packet);
                        }
                    }
                }
            }
        }
    }
}


void network_handle_client() {
    if (SDLNet_SocketReady(sd)) {
        if (SDLNet_UDP_Recv(sd, packet)) {
            int playerIndex, x, y;
            if (strcmp((char *)packet->data, "Server Full: No more connections allowed.") == 0) {
                printf("Failed to connect: %s\n", (char *)packet->data);
                return;
            }
            else if (sscanf((char *)packet->data, "%d,%d,%d", &playerIndex, &x, &y) == 3) {
                update_player_position(playerIndex, x, y);
                printf("Position update for player %d: x = %d, y = %d\n", playerIndex, x, y);
            } else {
                printf("Other message received: %s\n", (char *)packet->data);
            }
        }
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

void update_player_position(int playerIndex, int x, int y) {
    if (playerIndex >= 0 && playerIndex < MAX_CLIENTS) {
        clients[playerIndex].x = x;
        clients[playerIndex].y = y;
        printf("Updated position for client %d to (%d, %d)\n", playerIndex, x, y);
    }
}

void network_cleanup() {
    SDLNet_FreePacket(packet);
    SDLNet_UDP_Close(sd);
    SDLNet_Quit();
}