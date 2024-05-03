#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
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
    bool clientReady[MAX_PLAYERS];  // Array to track readiness of each client
    ServerData sData;

};
typedef struct game Game;

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
bool processClientData(Game *pGame, ClientData *data, IPaddress clientAddr);
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

    // Initialize TTF for text rendering
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF could not initialize: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }

    printf("Initializing SDL_net...\n");
    if (SDLNet_Init() != 0) {
        fprintf(stderr, "SDLNet could not initialize: %s\n", SDLNet_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    printf("Opening UDP socket...\n");
    pGame->pSocket = SDLNet_UDP_Open(1234);
    if (!pGame->pSocket) {
        fprintf(stderr, "Failed to open UDP socket: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        TTF_Quit();
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

    // Initialize client readiness states
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->clientReady[i] = false;
    }


    // Create SDL Window and Renderer
    pGame->pWindow = SDL_CreateWindow("Game Server", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, 0);
    if (!pGame->pWindow) {
        fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
        TTF_Quit();
        SDLNet_Quit();
        SDL_Quit();
        return 0;
    }

    pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pGame->pRenderer) {
        SDL_DestroyWindow(pGame->pWindow);
        fprintf(stderr, "Renderer could not be created: %s\n", SDL_GetError());
        TTF_Quit();
        SDLNet_Quit();
        SDL_Quit();
        return 0;
    }

    // Load a font
    TTF_Font *font = TTF_OpenFont("../lib/resources/Arial.ttf", 24);  // Make sure to have this font file available
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        TTF_Quit();
        SDLNet_Quit();
        SDL_Quit();
        return 0;
    }

    // Set text color
    SDL_Color textColor = {255, 255, 255}; // White
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Waiting for clients...", textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, textSurface);

    // Clear the renderer and draw the texture
    SDL_RenderClear(pGame->pRenderer);
    SDL_Rect textRect = { 100, 200, textSurface->w, textSurface->h };  // Position for the text
    SDL_RenderCopy(pGame->pRenderer, textTexture, NULL, &textRect);
    SDL_RenderPresent(pGame->pRenderer);

    // Clean up text objects
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);

    return 1; // Initialization successful
}


void run(Game *pGame) {
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Listen for incoming packets
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
            ClientData receivedData;
            memcpy(&receivedData, pGame->pPacket->data, sizeof(ClientData));
            printf("Received data from a client...\n");

            if (receivedData.command == CMD_READY) {
                // Process data and check if the client is now ready
                if (processClientData(pGame, &receivedData, pGame->pPacket->address)) {
                    // Display readiness state for all clients
                    printf("Current client states:\n");
                    for (int i = 0; i < pGame->nrOfClients; i++) {
                        printf("Client %d: %s\n", i + 1, pGame->clientReady[i] ? "Ready" : "Not Ready");
                    }

                    // Check if all clients are ready
                    bool allReady = true;
                    if (pGame->nrOfClients == MAX_PLAYERS) {
                        for (int i = 0; i < pGame->nrOfClients; i++) {
                            if (!pGame->clientReady[i]) {
                                allReady = false;
                                break;
                            }
                        }

                        // Check if all connected clients are ready and the maximum number of players has been reached
                        if (allReady && pGame->nrOfClients == MAX_PLAYERS) {
                            printf("All clients are ready. Starting the game...\n");
                            broadcastGameStart(pGame);
                            break;  // Exit the waiting loop
                        }
                    } else {
                        printf("Waiting for all clients to connect and signal readiness...\n");
                    }
                }
            }
        } else {
            //printf("No data received...\n");
        }
    }

    // Main game loop
    if (running) {
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }

            // Game logic goes here
            broadcastGameState(pGame);  // This function should manage and send game state updates
            SDL_Delay(16);  // Approximate frame delay for ~60 FPS
        }
    }
}





void close(Game *pGame) {
    if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);
    if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);
    SDLNet_Quit();
    SDL_Quit();
}


bool processClientData(Game *pGame, ClientData *data, IPaddress clientAddr) {
    int clientIndex = -1;
    // Check if this client is already connected
    for (int i = 0; i < pGame->nrOfClients; i++) {
        if (SDLNet_Read32(&clientAddr.host) == SDLNet_Read32(&pGame->clients[i].host) &&
            clientAddr.port == pGame->clients[i].port) {
            clientIndex = i;
            break;
        }
    }

    // Add new client if not already connected
    if (clientIndex == -1 && pGame->nrOfClients < MAX_PLAYERS) {
        clientIndex = pGame->nrOfClients;
        pGame->clients[clientIndex] = clientAddr;
        pGame->nrOfClients++;
        printf("New client connected. Total clients: %d\n", pGame->nrOfClients);
    }

    // Update client readiness state
    if (clientIndex != -1 && data->command == CMD_READY) {
        pGame->clientReady[clientIndex] = true;
        printf("Client %d is ready.\n", clientIndex + 1);

        // Check if all clients are ready
        bool allReady = true;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!pGame->clientReady[i]) {
                allReady = false;
                break;
            }
        }

        if (allReady && pGame->nrOfClients == MAX_PLAYERS) {
            printf("All clients are ready. Starting the game...\n");
            broadcastGameStart(pGame);
        }
        
        return true; // Client was updated
    }
    return false;
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

    pGame->sData.state = GAME_ONGOING;  // Update the game state to ongoing

    for (int i = 0; i < pGame->nrOfClients; i++) {
        pGame->pPacket->address = pGame->clients[i];
        SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);
        printf("Game start message sent to client %d\n", i + 1);
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
