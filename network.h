#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include <stdbool.h>

#define MAX_CLIENTS 4

typedef struct {
    IPaddress address;
    bool connected;
    int x, y;          // Player position
} ClientInfo;

int network_init(char* host, Uint16 port, bool isServer);
void network_check_activity();
void network_handle_server();
void network_handle_client();
int find_or_add_client(IPaddress newClientAddr);
void update_player_position(int playerIndex, int x, int y);
void network_cleanup();

#endif // NETWORK_H