#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_net.h>
#include "game_data.h"
#include "hunter.h"
#include "obstacle.h"
#include "sprinter.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define SERVER_PORT 1234


typedef struct {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    Player players[MAX_PLAYERS];
    SDL_Texture *backgroundTexture;
    SDL_Texture *hunterTexture;
    SDL_Texture *sprinterTexture;
    UDPsocket udpSocket;
    UDPpacket *packet;
    IPaddress clients[MAX_PLAYERS];
    int nrOfClients;
    ServerData sData;
} Game;

Obstacle obstacles[NUM_OBSTACLES];

// Function declarations
int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
int loadGameResources(SDL_Renderer *renderer, Game *pGame);
void renderPlayer(SDL_Renderer *renderer, Player *player);
void setupPlayerClips(Player *player);
void renderPlayers(Game *pGame);
void receiveData(Game *pGame);
void handlePlayerInput(Game *pGame, SDL_Event e, Player *player);
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles);
void sendPlayerMovement(Game *pGame, Player *player);
void updateFrame(int *frame, PlayerRole role, int frame1, int frame2);
bool checkCollision(SDL_Rect a, SDL_Rect b);

int main(int argc, char **argv) {
    Game g = {0};
    if (!initiate(&g)) return 1;
    run(&g);
    close(&g);
    return 0;
}

int initiate(Game *pGame) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
        return 0;
    }
    if (SDLNet_Init() != 0) {
        fprintf(stderr, "SDLNet could not initialize: %s\n", SDLNet_GetError());
        SDL_Quit();
        return 0;
    }

    pGame->pWindow = SDL_CreateWindow("Game Client", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!pGame->pWindow) {
        fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
        return 0;
    }

    pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pGame->pRenderer) {
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    if (!loadGameResources(pGame->pRenderer, pGame)) {
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        SDL_Quit();
        return 0;
    }

    // Setup network communication
    pGame->udpSocket = SDLNet_UDP_Open(0);  // Open a socket on any available port
    if (!pGame->udpSocket) {
        fprintf(stderr, "Failed to open UDP socket: %s\n", SDLNet_GetError());
        return 0;
    }

    // Setup packet for communication
    pGame->packet = SDLNet_AllocPacket(512);  // allocate a packet of size 512
    if (!pGame->packet) {
        fprintf(stderr, "Failed to allocate UDP packet: %s\n", SDLNet_GetError());
        return 0;
    }

    // Resolve server host
    if (SDLNet_ResolveHost(&pGame->clients[0], "127.0.0.1", SERVER_PORT) != 0) {
        fprintf(stderr, "Failed to resolve server address: %s\n", SDLNet_GetError());
        return 0;
    }

    // Initialize players and their properties
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->players[i].isActive = 1;
        pGame->players[i].texture = (i % 2 == 0) ? pGame->hunterTexture : pGame->sprinterTexture;
        setupPlayerClips(&pGame->players[i]);
        pGame->players[i].position = (SDL_Rect){100 + i * 100, 100, 32, 32};
    }

    return 1;
}

void run(Game *pGame) {
    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (pGame->players[i].isActive) {
                        handlePlayerInput(pGame, e, &pGame->players[i]);
                    }
                }
            }
        }
        receiveData(pGame);
        SDL_RenderClear(pGame->pRenderer);
        SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL);
        renderPlayers(pGame);
        SDL_RenderPresent(pGame->pRenderer);
        SDL_Delay(16);
    }
}


void close(Game *pGame) {
    if (pGame->packet) SDLNet_FreePacket(pGame->packet);
    if (pGame->udpSocket) SDLNet_UDP_Close(pGame->udpSocket);
    SDLNet_Quit();
    if (pGame->hunterTexture) SDL_DestroyTexture(pGame->hunterTexture);
    if (pGame->sprinterTexture) SDL_DestroyTexture(pGame->sprinterTexture);
    if (pGame->backgroundTexture) SDL_DestroyTexture(pGame->backgroundTexture);
    if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
    if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);
    SDL_Quit();
}

int loadGameResources(SDL_Renderer *renderer, Game *pGame) {
    SDL_Surface *bgSurface = IMG_Load("../lib/resources/Map.png");
    if (!bgSurface) {
        fprintf(stderr, "Failed to load background image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->backgroundTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface);

    SDL_Surface *hunterSurface = IMG_Load("../lib/resources/HUNTER.png");
    if (!hunterSurface) {
        fprintf(stderr, "Failed to load hunter image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->hunterTexture = SDL_CreateTextureFromSurface(renderer, hunterSurface);
    SDL_FreeSurface(hunterSurface);

    SDL_Surface *sprinterSurface = IMG_Load("../lib/resources/SPRINTER.png");
    if (!sprinterSurface) {
        fprintf(stderr, "Failed to load sprinter image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->sprinterTexture = SDL_CreateTextureFromSurface(renderer, sprinterSurface);
    SDL_FreeSurface(sprinterSurface);

    return 1;
}

void renderPlayer(SDL_Renderer *renderer, Player *player) {
    if (!player->isActive) return;
    SDL_Rect srcRect = player->spriteClips[player->currentFrame];
    SDL_Rect destRect = {player->position.x, player->position.y, player->position.w, player->position.h};
    SDL_RenderCopyEx(renderer, player->texture, &srcRect, &destRect, 0, NULL, SDL_FLIP_NONE);
}

void setupPlayerClips(Player *player) {
    for (int i = 0; i < 8; i++) {
        player->spriteClips[i] = (SDL_Rect){i * 16, 0, 16, 16};
        printf("Clip %d: x=%d, y=%d, w=%d, h=%d\n", i, player->spriteClips[i].x, player->spriteClips[i].y, player->spriteClips[i].w, player->spriteClips[i].h);
    }
}

void renderPlayers(Game *pGame) {
    for (int i = 0; i < 4; i++) {
        renderPlayer(pGame->pRenderer, &pGame->players[i]);
    }
}

void receiveData(Game *pGame) {
    if (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet)) {
        // Handle received data
        printf("Received data: %s\n", pGame->packet->data);
        // Update game state based on received data
    }
}

void handlePlayerInput(Game *pGame, SDL_Event e, Player *player) {
    int deltaX = 0, deltaY = 0;
    bool moved = false;

    switch (e.key.keysym.sym) {
        case SDLK_w: deltaY -= 8; moved = true; break;
        case SDLK_s: deltaY += 8; moved = true; break;
        case SDLK_a: deltaX -= 8; moved = true; break;
        case SDLK_d: deltaX += 8; moved = true; break;
    }

    if (moved) {
        moveCharacter(&player->position, deltaX, deltaY, player->type, obstacles, NUM_OBSTACLES);
        updateFrame(&player->currentFrame, player->type, 2, 3);
        sendPlayerMovement(pGame, player);
    }
}

// Function to move character with collision checking
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles) {
    SDL_Rect newPos = {charPos->x + deltaX, charPos->y + deltaY, charPos->w, charPos->h};

    for (int i = 0; i < numObstacles; i++) {
        if (checkCollision(newPos, obstacles[i].bounds)) {
            return;  // Collision detected, do not update position
        }
    }

    // Apply movement constraints (e.g., boundaries of the playing field)
    newPos.x = SDL_clamp(newPos.x, HORIZONTAL_MARGIN, WINDOW_WIDTH - newPos.w - HORIZONTAL_MARGIN);
    newPos.y = SDL_clamp(newPos.y, 0, WINDOW_HEIGHT - newPos.h);

    *charPos = newPos;
}


void sendPlayerMovement(Game *pGame, Player *player) {
    PlayerMovement move;
    move.playerId = player->playerId;
    move.x = player->position.x;
    move.y = player->position.y;

    memcpy(pGame->packet->data, &move, sizeof(PlayerMovement));
    pGame->packet->len = sizeof(PlayerMovement);
    SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
}


void updateFrame(int *frame, PlayerRole role, int frame1, int frame2) {
    *frame = (*frame == frame1) ? frame2 : frame1;
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
    // Check if there's no overlap
    if (a.x + a.w <= b.x || b.x + b.w <= a.x ||
        a.y + a.h <= b.y || b.y + b.h <= a.y) {
        return false;
    }
    return true;
}