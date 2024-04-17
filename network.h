// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include <stdbool.h>

int network_init(char* host, Uint16 port, bool isServer);
void network_check_activity(Uint32 timeout);
void network_handle_server();
void network_handle_client();
void network_cleanup();

#endif // NETWORK_H