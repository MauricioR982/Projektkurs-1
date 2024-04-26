#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#define MAX_CLIENTS 4
#define PORT 12345
#define BUFFER_SIZE 512

typedef struct {
    UDPsocket socket;
    UDPpacket *packet;
} GameServer;

int init_server(GameServer *server) {
    if (SDLNet_Init() < 0) {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        return -1;
    }

    server->socket = SDLNet_UDP_Open(PORT);
    if (!server->socket) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        return -1;
    }

    server->packet = SDLNet_AllocPacket(BUFFER_SIZE);
    if (!server->packet) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(server->socket);
        return -1;
    }

    return 0;
}

void handle_packets(GameServer *server) {
    while (SDLNet_UDP_Recv(server->socket, server->packet)) {
        printf("Received packet from %x %d containing: %s\n",
               SDLNet_Read32(&server->packet->address.host),
               SDLNet_Read16(&server->packet->address.port),
               (char *)server->packet->data);

        // Echo the packet back to the sender
        SDLNet_UDP_Send(server->socket, -1, server->packet);  // -1 means send to all
    }
}

void cleanup_server(GameServer *server) {
    SDLNet_FreePacket(server->packet);
    SDLNet_UDP_Close(server->socket);
    SDLNet_Quit();
}

int main(int argc, char **argv) {
    GameServer server;

    if (init_server(&server) == -1) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    printf("Server started on port %d\n", PORT);
    while (1) {
        handle_packets(&server);
    }

    cleanup_server(&server);
    return 0;
}
