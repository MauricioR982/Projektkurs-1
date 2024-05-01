#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include "game_data.h"
#include "hunter.h"
#include "obstacle.h"
#include "sprinter.h"

struct game {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    int nrOfPlayers;



    UDPsocket pSocket;
	UDPpacket *pPacket;
    IPaddress clients[MAX_PLAYERS];
    int nrOfClients;
    ServerData sData;

};
typedef struct game Game;

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
void processClientData(Game *pGame, ClientData *data, IPaddress clientAddr);
void updatePlayerState(Game *pGame, ClientData *data, IPaddress clientAddr);
void broadcastGameState(Game *pGame);
void updateGameState(Game *pGame);
void broadcastGameStart(Game *pGame);


int main(int argv, char** args){
    Game g={0};
    if(!initiate(&g)) return 1;
    run(&g);
    close(&g);

    return 0;
}

int initiate(Game *pGame) {
    printf("Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
        return 0;
    }
    printf("Initializing SDL_net...\n");
    if (SDLNet_Init() != 0) {
        fprintf(stderr, "SDLNet could not initialize: %s\n", SDLNet_GetError());
        SDL_Quit();
        return 0;
    }
    printf("Opening UDP socket...\n");
    pGame->pSocket = SDLNet_UDP_Open(1234);
    if (!pGame->pSocket) {
        fprintf(stderr, "Failed to open UDP socket: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 0;
    } else {
        printf("UDP socket opened successfully on port 1234.\n");
    }
    printf("Allocating UDP packet...\n");
    pGame->pPacket = SDLNet_AllocPacket(512);
    if (!pGame->pPacket) {
        fprintf(stderr, "Failed to allocate UDP packet: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(pGame->pSocket);
        SDLNet_Quit();
        SDL_Quit();
        return 0;
    }
    return 1; // Initialization successful
}


void run(Game *pGame) {
    printf("Server is running. Waiting for at least 2 clients to connect...\n");
    bool running = true;
    SDL_Event event;

    while (running) {
        // Check for events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        // Existing UDP receive code
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
            printf("Received packet from %x %hu\n",
                   pGame->pPacket->address.host, pGame->pPacket->address.port);
            // Processing of the packet
        } else {
            printf("No packets received. Waiting...\n");
        }

        SDL_Delay(10);  // Reduce CPU usage
    }

    printf("Server shutting down...\n");
}









void close(Game *pGame) {
    if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);
    if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);
    SDLNet_Quit();
    SDL_Quit();
}


void processClientData(Game *pGame, ClientData *data, IPaddress clientAddr) {
    // Check if this client is already added
    bool clientExists = false;
    for (int i = 0; i < pGame->nrOfClients; i++) {
        if (SDLNet_Read32(&clientAddr.host) == SDLNet_Read32(&pGame->clients[i].host) &&
            clientAddr.port == pGame->clients[i].port) {
            clientExists = true;
            break;
        }
    }

    // If new client, add to list
    if (!clientExists && pGame->nrOfClients < MAX_PLAYERS) {
        pGame->clients[pGame->nrOfClients] = clientAddr;
        pGame->nrOfClients++;
        printf("New client added. Total clients: %d\n", pGame->nrOfClients);
    }

    // Update player state based on command
    updatePlayerState(pGame, data, clientAddr);
}


void updatePlayerState(Game *pGame, ClientData *data, IPaddress clientAddr) {
    // Find the player associated with this IP and update their state
    for (int i = 0; i < pGame->nrOfClients; ++i) {
        if (SDLNet_Read32(&clientAddr.host) == SDLNet_Read32(&pGame->clients[i].host) &&
            clientAddr.port == pGame->clients[i].port) {
            // Update player position or state here based on data->command
            break;
        }
    }
}

void updateGameState(Game *pGame) {
    // Update your game logic here, e.g., checking for collisions or game rules
}

void broadcastGameStart(Game *pGame) {
    const char *startMsg = "Game Start";
    memcpy(pGame->pPacket->data, startMsg, strlen(startMsg) + 1);
    pGame->pPacket->len = strlen(startMsg) + 1;

    for (int i = 0; i < pGame->nrOfClients; ++i) {
        pGame->pPacket->address = pGame->clients[i];
        SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);
    }
}


void broadcastGameState(Game *pGame) {
    if (!pGame->pPacket) return;

    // Example: broadcasting a simple "Hello" message
    const char* message = "Game State Update";
    memcpy(pGame->pPacket->data, message, strlen(message) + 1);  // Include null terminator
    pGame->pPacket->len = strlen(message) + 1;

    for (int i = 0; i < pGame->nrOfClients; i++) {
        pGame->pPacket->address = pGame->clients[i];
        SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);
    }
}
