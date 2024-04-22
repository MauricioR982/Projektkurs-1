//
// network.c
// Developed by Grupp 10 - Datateknik, project-start at 2024-03
//

#include <SDL2/SDL.h>
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
static bool isServerRunning = true; // Add a global variable to control server running state
bool serverConnected = false;


int network_init(char* host, Uint16 port, bool serverMode) {
    isServer = serverMode;
    printf("Initializing network. Server Mode: %d, Host: %s, Port: %d\n", isServer, host, port);

    if (SDLNet_Init() < 0) {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        return -1;
    }

    packet = SDLNet_AllocPacket(512);
    if (!packet) {
        fprintf(stderr, "SDLNet_AllocPacket failed: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        return -1;
    }

    if (isServer) {
        sd = SDLNet_UDP_Open(port);
        if (!sd) {
            fprintf(stderr, "Failed to open UDP socket on port %d: %s\n", port, SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        printf("Server initialized on port %d\n", port);
    } else {
        if (SDLNet_ResolveHost(&srvadd, host, port) < 0) {
            fprintf(stderr, "SDLNet_ResolveHost failed for host %s:%d: %s\n", host, port, SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        sd = SDLNet_UDP_Open(0); // Open on a random port for the client
        if (!sd) {
            fprintf(stderr, "Failed to open UDP socket on client: %s\n", SDLNet_GetError());
            SDLNet_FreePacket(packet);
            SDLNet_Quit();
            return -1;
        }
        printf("Client initialized and trying to connect to %s:%d\n", host, port);
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
    int numready;
    SDLNet_SocketSet set = SDLNet_AllocSocketSet(1);
    SDLNet_UDP_AddSocket(set, sd);

    printf("Server started handling.\n");
    // Reduce to just checking once per call to avoid freezing
    numready = SDLNet_CheckSockets(set, 0); // Check immediately, non-blocking
    if (numready > 0 && SDLNet_SocketReady(sd)) {
        UDPpacket *recvPacket = SDLNet_AllocPacket(512);
        if (recvPacket) {
            if (SDLNet_UDP_Recv(sd, recvPacket)) {
                printf("Received packet\n");
                // Process packet here, update game state
            }
            SDLNet_FreePacket(recvPacket);
        }
    }
    SDLNet_FreeSocketSet(set);
}

void network_handle_client() {
    if (!serverConnected) {
        check_server_connection();
        return;
    }

    UDPpacket *recvPacket = SDLNet_AllocPacket(512);
    if (recvPacket == NULL) {
        fprintf(stderr, "Failed to allocate packet for receiving\n");
        return;
    }

    // Listen for incoming packets and process them
    if (SDLNet_UDP_Recv(sd, recvPacket)) {
        printf("Received packet from server.\n");
        handle_server_response(recvPacket);  // Process each received packet
    }

    SDLNet_FreePacket(recvPacket);
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
    if (isServer) {
        printf("Server stopped handling\n"); // Add this debug statement
    }
    SDLNet_FreePacket(packet);
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
    static Uint32 lastSendTime = 0; // Keep track of the last send time
    Uint32 currentTime = SDL_GetTicks(); // Get the current time in milliseconds
    Uint32 sendInterval = 100; // Set an interval in milliseconds (e.g., 100ms between sends)

    if (currentTime - lastSendTime < sendInterval) {
        return; // Skip sending if the interval hasn't passed
    }

    lastSendTime = currentTime; // Update last send time

    PlayerState localState = {0}; // Initialize with your own logic to populate player state
    serialize_player_state(&localState, packet); // Prepare packet data
    packet->address = srvadd; // Set server address for the packet

    if (SDLNet_UDP_Send(sd, -1, packet) == 0) {
        fprintf(stderr, "Failed to send packet: %s\n", SDLNet_GetError());
    } else {
        // Use SDLNet_ResolveIP to get the IP address in a readable format
        const char *ip = SDLNet_ResolveIP(&packet->address);
        if (ip != NULL) {
            printf("Packet sent to %s:%d\n", ip, SDLNet_Read16(&srvadd.port));
        } else {
            printf("Packet sent to unknown IP:%d\n", SDLNet_Read16(&srvadd.port));
        }
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

void check_server_connection() {
    // Send a heartbeat or status check packet
    PlayerState statusCheck = { .playerIndex = -1 }; // Special packet to check status
    serialize_player_state(&statusCheck, packet);
    packet->address = srvadd;

    if (SDLNet_UDP_Send(sd, -1, packet) == 0) {
        fprintf(stderr, "Connection check failed: %s\n", SDLNet_GetError());
        serverConnected = false;  // Update connection status
    }
}

void server_send_acknowledge(UDPsocket sd, IPaddress clientAddr) {
    UDPpacket *ackPacket = SDLNet_AllocPacket(512);
    if (!ackPacket) {
        fprintf(stderr, "Failed to allocate packet for acknowledgment\n");
        return;
    }
    
    // Prepare the acknowledgment packet
    const char* ackMessage = "Server ACK";  // Define a simple acknowledgment message
    memcpy(ackPacket->data, ackMessage, strlen(ackMessage) + 1);
    ackPacket->len = strlen(ackMessage) + 1;
    ackPacket->address = clientAddr;

    // Send the acknowledgment
    if (SDLNet_UDP_Send(sd, -1, ackPacket) == 0) {
        fprintf(stderr, "Failed to send acknowledgment: %s\n", SDLNet_GetError());
    }

    SDLNet_FreePacket(ackPacket);
}

void handle_server_response(UDPpacket *packet) {
    const char* expectedAck = "Server ACK";  // The expected acknowledgment message
    if (packet->len > 0 && strncmp((char*)packet->data, expectedAck, packet->len) == 0) {
        serverConnected = true;
        printf("Acknowledgment from server received. Server is up.\n");
    }
}