#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
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
    IPaddress serverAddress;
    GameState state;
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
void handlePlayerInput(Game *pGame, SDL_Event e);
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles);
void sendPlayerMovement(Game *pGame, Player *player);
void updateFrame(int *frame, PlayerRole role, int frame1, int frame2);
bool checkCollision(SDL_Rect a, SDL_Rect b);
void startGame(Game *pGame);

int main(int argc, char **argv) {
    Game g = {0};
    if (!initiate(&g)) return 1;
    run(&g);
    close(&g);
    return 0;
}

int initiate(Game *pGame) {
    // Initialize SDL, SDL_ttf, and SDL_net
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
        return 0;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF could not initialize: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    if (SDLNet_Init() != 0) {
        fprintf(stderr, "SDLNet could not initialize: %s\n", SDLNet_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    // Create Window and Renderer
    pGame->pWindow = SDL_CreateWindow("Game Client", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pGame->pWindow || !pGame->pRenderer) {
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    // Load game resources, including fonts
    if (!loadGameResources(pGame->pRenderer, pGame)) {
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    // Network setup
    pGame->udpSocket = SDLNet_UDP_Open(0);  // Open a socket on any available port
    if (!pGame->udpSocket) {
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    pGame->packet = SDLNet_AllocPacket(512);  // Allocate a packet of size 512
    if (!pGame->packet) {
        SDLNet_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    if (SDLNet_ResolveHost(&pGame->serverAddress, "127.0.0.1", SERVER_PORT) == -1) {
        fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    pGame->packet->address.host = pGame->serverAddress.host; // Important: Ensure the packet knows where to go
    pGame->packet->address.port = pGame->serverAddress.port; // Important: Ensure the packet knows where to go

    // Prepare the packet to send
    ClientData data = {CMD_READY, -1};  // Your actual player ID should replace -1
    memcpy(pGame->packet->data, &data, sizeof(ClientData));
    pGame->packet->len = sizeof(ClientData);

    // Send the packet
    if (SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet) == 0) {
        fprintf(stderr, "SDLNet_UDP_Send: %s\n", SDLNet_GetError());
    }


    // Initialize players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->players[i].isActive = 1;
        pGame->players[i].texture = (i % 2 == 0) ? pGame->hunterTexture : pGame->sprinterTexture;
        setupPlayerClips(&pGame->players[i]);
        pGame->players[i].position = (SDL_Rect){100 + i * 100, 100, 32, 32};
    }

    // Set initial game state
    pGame->state = GAME_WAITING;
    return 1;
}

void run(Game *pGame) {
    bool running = true;
    SDL_Event e;

    // Setup for text rendering
    SDL_Color textColor = {255, 255, 255};  // White color for the text
    TTF_Font* font = TTF_OpenFont("../lib/resources/arial.ttf", 28); // Ensure the path is correct

    // Text for initial prompt
    SDL_Surface* initialSurface = TTF_RenderText_Solid(font, "Press space to connect to the server", textColor);
    SDL_Texture* initialTextTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, initialSurface);
    SDL_Rect initialTextRect = {50, 50, initialSurface->w, initialSurface->h};
    SDL_FreeSurface(initialSurface);

    // Text for waiting message
    SDL_Surface* waitingSurface = TTF_RenderText_Solid(font, "Waiting for server...", textColor);
    SDL_Texture* waitingTextTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, waitingSurface);
    SDL_Rect waitingTextRect = {50, 100, waitingSurface->w, waitingSurface->h};
    SDL_FreeSurface(waitingSurface);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_SPACE && pGame->state == GAME_WAITING) {
                    pGame->state = GAME_READY;
                    ClientData data = {CMD_READY, -1};
                    memcpy(pGame->packet->data, &data, sizeof(ClientData));
                    pGame->packet->len = sizeof(ClientData);
                    if (SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet) < 1) {
                        fprintf(stderr, "Failed to send packet: %s\n", SDLNet_GetError());
                    } else {
                        printf("Packet sent. Length: %d\n", pGame->packet->len);
                    }

                }
            }
        }

        // Listen for server commands
        if (pGame->state == GAME_READY || pGame->state == GAME_ONGOING) {
            receiveData(pGame); // This function must be polled frequently.
        }

        // Rendering based on game state
        SDL_RenderClear(pGame->pRenderer);
        if (pGame->state == GAME_ONGOING) {
            renderPlayers(pGame); // Render all active players
        } else if (pGame->state == GAME_READY) {
            SDL_RenderCopy(pGame->pRenderer, waitingTextTexture, NULL, &waitingTextRect);
        } else if (pGame->state == GAME_WAITING) {
            SDL_RenderCopy(pGame->pRenderer, initialTextTexture, NULL, &initialTextRect);
        }
        SDL_RenderPresent(pGame->pRenderer);
        SDL_Delay(16); // simulate frame update roughly every 16ms (about 60fps)
    }

    // Clean up resources
    SDL_DestroyTexture(initialTextTexture);
    SDL_DestroyTexture(waitingTextTexture);
    TTF_CloseFont(font);
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
        //printf("Clip %d: x=%d, y=%d, w=%d, h=%d\n", i, player->spriteClips[i].x, player->spriteClips[i].y, player->spriteClips[i].w, player->spriteClips[i].h);
    }
}

void renderPlayers(Game *pGame) {
    for (int i = 0; i < 4; i++) {
        renderPlayer(pGame->pRenderer, &pGame->players[i]);
    }
}

void receiveData(Game *pGame) {
    while (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet)) {
        printf("Data received: %s\n", (char*)pGame->packet->data);

        if (strcmp((char*)pGame->packet->data, "Game Start") == 0) {
            printf("Received 'Game Start' message from server.\n");
            pGame->state = GAME_ONGOING;  // Only transition to GAME_ONGOING when this message is received
            startGame(pGame);
        }
    }
}




void handlePlayerInput(Game *pGame, SDL_Event e) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (pGame->players[i].isActive) {
            int deltaX = 0, deltaY = 0;
            bool moved = false;

            switch (e.key.keysym.sym) {
                case SDLK_w: deltaY -= 8; moved = true; break;
                case SDLK_s: deltaY += 8; moved = true; break;
                case SDLK_a: deltaX -= 8; moved = true; break;
                case SDLK_d: deltaX += 8; moved = true; break;
            }

            if (moved) {
                moveCharacter(&pGame->players[i].position, deltaX, deltaY, pGame->players[i].type, obstacles, NUM_OBSTACLES);
                updateFrame(&pGame->players[i].currentFrame, pGame->players[i].type, 2, 3);
                sendPlayerMovement(pGame, &pGame->players[i]);
            }
        }
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
    pGame->packet->address = pGame->serverAddress;  // Make sure this is set correctly each time

    if (SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet) < 1) {
        printf("Trying to send data to server...\n");
        fprintf(stderr, "Failed to send packet: %s\n", SDLNet_GetError());
    } else {
        printf("Packet sent to server: Player %d at (%d, %d)\n", move.playerId, move.x, move.y);
    }
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

void startGame(Game *pGame) {
    printf("Game starting!\n");
    pGame->state = GAME_ONGOING; // Set game state to ongoing

    // Ensure all players are properly initialized and set as active
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->players[i].isActive = true; // Ensure all players are active at start
        setupPlayerClips(&pGame->players[i]); // Setup sprite clips if not already done
    }

    bool running = true;
    SDL_Event e;

    // Game loop
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false; // Exit the loop if window is closed
            } else if (e.type == SDL_KEYDOWN) {
                handlePlayerInput(pGame, e); // Handle player inputs for all players
            }
        }

        // Update game state here if necessary, such as moving NPCs or handling game logic

        // Render the game state
        SDL_RenderClear(pGame->pRenderer);
        SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL); // Draw the background
        renderPlayers(pGame); // Draw all players
        SDL_RenderPresent(pGame->pRenderer);

        SDL_Delay(16); // Delay to cap frame rate at about 60 fps
    }
}

