// network.c

#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static UDPsocket sd; // Socket descriptor
static IPaddress srvadd; // Server address
static bool isServer;
static UDPpacket *packet;

int network_init(char* host, Uint16 port, bool serverMode) {
    isServer = serverMode;
    
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

    // Check the socket for activity
    if (SDLNet_SocketReady(sd)) {
        if (SDLNet_UDP_Recv(sd, packet)) {
            // Process the packet data
            printf("Received packet from %x %x containing: %s\n",
                   packet->address.host, packet->address.port, (char *)packet->data);

            // Send a reply (optional)
            const char* reply = "Server Ack";
            memcpy(packet->data, reply, strlen(reply) + 1);
            packet->len = strlen(reply) + 1;
            SDLNet_UDP_Send(sd, -1, packet); // send to the packet's sender
        }
    }
}

void network_handle_client() {

    // Check the socket for activity
    if (SDLNet_SocketReady(sd)) {
        if (SDLNet_UDP_Recv(sd, packet)) {
            printf("Received packet from server containing: %s\n", (char *)packet->data);
        }
    }

    // Sending data to the server could be done here or triggered by game events
    // Example of sending a message to the server
    const char* message = "Hello Server";
    memcpy(packet->data, message, strlen(message) + 1);
    packet->len = strlen(message) + 1;
    packet->address = srvadd; // set the packet's destination
    if (!SDLNet_UDP_Send(sd, -1, packet)) {
        printf("SDLNet_UDP_Send: %s\n", SDLNet_GetError());
    }
}


void network_cleanup() {
    SDLNet_FreePacket(packet);
    SDLNet_UDP_Close(sd);
    SDLNet_Quit();
}
