#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include "game_data.h"
#include "hunter.h"
#include "obstacle.h"
#include "sprinter.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAX_PLAYERS 2

struct game
{
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;

    GameState state;

    UDPsocket pSocket;
    IPaddress serverAddress;
    UDPpacket *pPacket;

    SDL_Texture *backgroundTexture;
    SDL_Texture *hunterTexture;
    SDL_Texture *sprinterTexture;
    
};
typedef struct game Game;

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
int loadGameResources(SDL_Renderer *renderer, Game *pGame);

int main(int argv, char** args){
    Game g = {0};
    if(!initiate(&g)) return 1;
    run(&g);
    close(&g);

    return 0;
}

#include <SDL2/SDL_net.h>

int initiate(Game *pGame) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Create window
    pGame->pWindow = SDL_CreateWindow("Game Client", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (pGame->pWindow == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    // Create renderer
    pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (pGame->pRenderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    // Initialize SDL_net
    if (SDLNet_Init() == -1) {
        fprintf(stderr, "SDLNet could not initialize! SDLNet_Error: %s\n", SDLNet_GetError());
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    // Create a UDP socket
    pGame->pSocket = SDLNet_UDP_Open(0);  // Use 0 if the port is not important or a specific port number otherwise
    if (pGame->pSocket == NULL) {
        fprintf(stderr, "Could not create UDP socket! SDLNet_Error: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    // Resolve server address
    if (SDLNet_ResolveHost(&pGame->serverAddress, "127.0.0.1", 2000) == -1) {
        fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(pGame->pSocket);
        SDLNet_Quit();
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    // Allocate memory for the packet
    pGame->pPacket = SDLNet_AllocPacket(512);  // Define packet size that fits your need
    if (pGame->pPacket == NULL) {
        fprintf(stderr, "Could not allocate UDP packet! SDLNet_Error: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(pGame->pSocket);
        SDLNet_Quit();
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    if (!loadGameResources(pGame->pRenderer, pGame)) {
        fprintf(stderr, "Failed to load game resources\n");
        // Perform cleanup if necessary
        return 0;
    }


    return 1;
}


void run(Game *pGame) {
    bool running = true;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            // Handle other events such as keyboard inputs
        }

        // Clear screen
        SDL_RenderClear(pGame->pRenderer);

        // Render background
        SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL);

        // Logic to render hunter and sprinter goes here...

        // Update screen
        SDL_RenderPresent(pGame->pRenderer);

        SDL_Delay(16); // ~60 frames per second
    }
}


void close(Game *pGame) {
    if (pGame->hunterTexture) {
        SDL_DestroyTexture(pGame->hunterTexture);
    }
    if (pGame->sprinterTexture) {
        SDL_DestroyTexture(pGame->sprinterTexture);
    }
    if (pGame->backgroundTexture) {
        SDL_DestroyTexture(pGame->backgroundTexture);
    }
    if (pGame->pRenderer) {
        SDL_DestroyRenderer(pGame->pRenderer);
    }
    if (pGame->pWindow) {
        SDL_DestroyWindow(pGame->pWindow);
    }
    if (pGame->pPacket) {
        SDLNet_FreePacket(pGame->pPacket);
    }
    if (pGame->pSocket) {
        SDLNet_UDP_Close(pGame->pSocket);
    }
    SDLNet_Quit();
    SDL_Quit();
}


int loadGameResources(SDL_Renderer *renderer, Game *pGame) {
    // Load background
    SDL_Surface *bgSurface = IMG_Load("../lib/resources/Map.png");
    if (!bgSurface) {
        fprintf(stderr, "Failed to load background image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->backgroundTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface);
    if (!pGame->backgroundTexture) {
        fprintf(stderr, "Failed to create texture from background image: %s\n", SDL_GetError());
        return 0;
    }

    // Load hunter
    SDL_Surface *hunterSurface = IMG_Load("../lib/resources/HUNTER.png");
    if (!hunterSurface) {
        fprintf(stderr, "Failed to load hunter image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->hunterTexture = SDL_CreateTextureFromSurface(renderer, hunterSurface);
    SDL_FreeSurface(hunterSurface);
    if (!pGame->hunterTexture) {
        fprintf(stderr, "Failed to create texture from hunter image: %s\n", SDL_GetError());
        return 0;
    }

    // Load sprinter
    SDL_Surface *sprinterSurface = IMG_Load("../lib/resources/SPRINTER.png");
    if (!sprinterSurface) {
        fprintf(stderr, "Failed to load sprinter image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->sprinterTexture = SDL_CreateTextureFromSurface(renderer, sprinterSurface);
    SDL_FreeSurface(sprinterSurface);
    if (!pGame->sprinterTexture) {
        fprintf(stderr, "Failed to create texture from sprinter image: %s\n", SDL_GetError());
        return 0;
    }

    return 1;
}

