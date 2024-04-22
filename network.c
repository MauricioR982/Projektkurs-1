//
// network.c
// Developed by Grupp 10 - Datateknik, project-start at 2024-03
//

#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "game_types.h"


static UDPsocket sd; // Socket descriptor
static IPaddress srvadd; // Server address
static bool isServer;
static UDPpacket *packet;
static ClientInfo clients[MAX_CLIENTS];
extern Player players[MAX_CLIENTS];  // Declare the external linkage


int network_init(char* host, Uint16 port, bool serverMode) {
    isServer = serverMode;
    printf("Initializing network. Server Mode: %d, Host: %s, Port: %d\n", isServer, host, port);

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
        // Open a socket on a specific port for server
        sd = SDLNet_UDP_Open(port);
        if (!sd) {
            fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        printf("Server listening on port %d\n", port);
    } else {
        // Resolve server name and open a socket on random port for client
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
    SDLNet_SocketSet set = SDLNet_AllocSocketSet(1);
    SDLNet_UDP_AddSocket(set, sd);

    while (true) {  // Consider implementing a way to exit this loop properly.
        int numready = SDLNet_CheckSockets(set, 10); // Checks every 10 milliseconds.
        if (numready > 0 && SDLNet_SocketReady(sd)) {
            if (SDLNet_UDP_Recv(sd, packet)) {
                printf("Received packet.\n");
                // Handle the packet
            }
        } else {
            SDL_Delay(10); // Reduce CPU usage if no data is incoming.
        }
    }

    SDLNet_FreeSocketSet(set);
}

void network_handle_client() {
    // Regularly send local state to the server
    send_local_player_state();

    // Check for incoming messages
    if (SDLNet_UDP_Recv(sd, packet)) {
        PlayerState state;
        deserialize_player_state(&state, packet);  // Deserialize data before processing
        process_incoming_state(&state);  // Pass the deserialized state
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

void serialize_player_state(PlayerState *state, UDPpacket *packet) {
    // Ensure the packet has enough space
    int *data = (int*)packet->data;
    data[0] = state->playerIndex;
    data[1] = state->x;
    data[2] = state->y;
    packet->len = 3 * sizeof(int);
}

// Example deserialization function
void deserialize_player_state(PlayerState *state, UDPpacket *packet) {
    int *data = (int*)packet->data;
    state->playerIndex = data[0];
    state->x = data[1];
    state->y = data[2];
}

void send_local_player_state() {
    PlayerState localState = {0}; // Assume you have some way to get local player state
    // Populate localState with the current player's data

    if (SDLNet_UDP_Send(sd, -1, packet) == 0) {
        fprintf(stderr, "Failed to send packet: %s\n", SDLNet_GetError());
    }
}

void process_incoming_state(const PlayerState *state) {
    // Here you would update the game state based on the incoming data
    update_player_position(state->playerIndex, state->x, state->y);
}

void update_player_position(int playerIndex, int x, int y) {
    // Assuming `players` is an array of Player structs
    // Check for valid index before assignment to prevent out-of-bounds error
    if (playerIndex >= 0 && playerIndex < MAX_CLIENTS) {
        players[playerIndex].x = x;
        players[playerIndex].y = y;
        // Additional logic to handle changes in player state, like updating the renderer or game logic
    }
}



