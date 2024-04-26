#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BUFFER_SIZE 512

typedef struct {
    UDPsocket socket;
    IPaddress server_ip;
    UDPpacket *packet;
} GameClient;

int init_client(GameClient *client) {
    if (SDLNet_Init() < 0) {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        return -1;
    }

    client->socket = SDLNet_UDP_Open(0); // 0 means dynamic port
    if (!client->socket) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        return -1;
    }

    if (SDLNet_ResolveHost(&client->server_ip, SERVER_IP, SERVER_PORT) == -1) {
        fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(client->socket);
        return -1;
    }

    client->packet = SDLNet_AllocPacket(BUFFER_SIZE);
    if (!client->packet) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(client->socket);
        return -1;
    }

    return 0;
}

void send_data(GameClient *client, const char *data) {
    strcpy((char *)client->packet->data, data);
    client->packet->address = client->server_ip;
    client->packet->len = strlen(data) + 1;

    SDLNet_UDP_Send(client->socket, -1, client->packet);
}

void receive_data(GameClient *client) {
    if (SDLNet_UDP_Recv(client->socket, client->packet)) {
        printf("Received: %s\n", (char *)client->packet->data);
    }
}

void cleanup_client(GameClient *client) {
    SDLNet_FreePacket(client->packet);
    SDLNet_UDP_Close(client->socket);
    SDLNet_Quit();
}

int main(int argc, char **argv) {
    GameClient client;

    if (init_client(&client) == -1) {
        fprintf(stderr, "Failed to initialize client\n");
        return 1;
    }

    send_data(&client, "Hello, server!");
    while (1) {
        receive_data(&client);
    }

    cleanup_client(&client);
    return 0;
}
