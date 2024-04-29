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
#define HORIZONTAL_MARGIN 20 // Left & right boundary collision
#define NUM_OBSTACLES 23
#define MAX_PLAYERS 2

typedef struct {
    int x;
    int y;
} sPosition;

typedef struct {
    int x;
    int y;
} hPosition; // Hunter spawn position

typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;

typedef struct {
    int playerId;          // Unique identifier for each player
    SDL_Rect position;
    PlayerRole role;
    SDL_Texture *texture;
    SDL_Rect spriteClips[8];
    SDL_RendererFlip flip;
    int currentFrame;
} Player;

GameState current_state;

const int arrowYPositions[] = {100, 198, 288}; // Y-positions for our menu-options
Obstacle obstacles[NUM_OBSTACLES];

struct game
{
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    SDL_Texture *mSprinter, *mHunter, *mBackground, *mMenu, *mArrow;
    SDL_Rect gSprinterSpriteClips[8], gHunterSpriteClips[8];
    UDPsocket pSocket;
    IPaddress serverAddress;
    UDPpacket *pPacket;
    Player players[MAX_PLAYERS];
    bool quit;
};
typedef struct game Game;

bool init(SDL_Renderer **gRenderer);
void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSprinterSpriteClips[], SDL_Texture **mHunter, SDL_Rect gHunterSpriteClips[], SDL_Texture **mBackground, SDL_Texture **mMenu, SDL_Texture **mArrow);
void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mBackground);
void handlePlayerInput(SDL_Event e, Player *player, Game *pGame);
void renderPlayers(SDL_Renderer *gRenderer, Game *pGame);
void close(Game *pGame);
int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int main(int argv, char** args){
    Game g = {0};
    if(!initiate(&g)) return 1;
    run(&g);
    close(&g);

    return 0;
}

int initiate(Game *pGame) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        SDL_Quit();
        return 0;
    }
    pGame->pWindow = SDL_CreateWindow("Network Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
    pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pGame->pWindow || !pGame->pRenderer) {
        fprintf(stderr, "Window or renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    loadMedia(pGame->pRenderer, &pGame->mSprinter, pGame->gSprinterSpriteClips, &pGame->mHunter, pGame->gHunterSpriteClips, &pGame->mBackground, &pGame->mMenu, &pGame->mArrow);
    // Initialize SDL_net
    if (SDLNet_Init() == -1) {
        fprintf(stderr, "SDLNet could not initialize! SDLNet_Error: %s\n", SDLNet_GetError());
        return 0;
    }
    // Create UDP socket and set up the server address
    pGame->pSocket = SDLNet_UDP_Open(0);  // 0 if client
    SDLNet_ResolveHost(&pGame->serverAddress, "localhost", 1234);  // Server IP and port
    pGame->pPacket = SDLNet_AllocPacket(512);  // Adjust size as needed
    if (!pGame->pSocket || !pGame->pPacket) {
        fprintf(stderr, "Failed to open UDP socket or packet! SDLNet_Error: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}

void run(Game *pGame) {
    SDL_Event e;
    pGame->quit = false;
    while (!pGame->quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                pGame->quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                handlePlayerInput(e, &pGame->players[0], pGame);  // Assuming player[0] is the local player
            }
        }
        SDL_RenderClear(pGame->pRenderer);
        renderBackground(pGame->pRenderer, pGame->mBackground);
        renderPlayers(pGame->pRenderer, pGame);
        SDL_RenderPresent(pGame->pRenderer);
        SDL_Delay(16);  // About 60 FPS
    }
}

void close(Game *pGame) {
    SDL_DestroyRenderer(pGame->pRenderer);
    SDL_DestroyWindow(pGame->pWindow);
    SDLNet_FreePacket(pGame->pPacket);
    SDLNet_UDP_Close(pGame->pSocket);
    SDLNet_Quit();
    Mix_CloseAudio();
    SDL_Quit();
}
