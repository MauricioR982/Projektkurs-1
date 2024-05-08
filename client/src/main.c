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
#include "text.h"

typedef struct {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    Player players[MAX_PLAYERS];
    int playerNr;
    SDL_Texture *backgroundTexture, *hunterTexture, *sprinterTexture, *initialTextTexture;
    UDPsocket udpSocket;
    UDPpacket *packet;
    IPaddress serverAddress;
    GameState state;
    TTF_Font *pFont;
    Text *pWaitingText, *pJoinText;
} Game;

Obstacle obstacles[NUM_OBSTACLES];
Hunter hunter;  // One hunter
Sprinter sprinters[MAX_PLAYERS - 1]; //Remaining players become sprinters

// Function declarations
int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
int loadGameResources(SDL_Renderer *renderer, Game *pGame);
void renderPlayer(SDL_Renderer *renderer, Player *player);
void setupPlayerClips(Player *player);
void renderPlayers(Game *pGame);
void handlePlayerInput(Game *pGame, SDL_Event *pEvent);
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles);
void updateFrame(int *frame, PlayerRole role, int frame1, int frame2);
bool checkCollision(SDL_Rect a, SDL_Rect b);
void updateWithServerData(Game *pGAme);
void initializePlayers(Game *pGame);

int main(int argc, char **argv) {
    Game g = {0};
    if (!initiate(&g)) return 1;
    run(&g);
    close(&g);
    return 0;
}

int initiate(Game *pGame) {
    
    // Initialize SDL, SDL_ttf, and SDL_net
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
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

    pGame->pFont = TTF_OpenFont("../lib/resources/arial.ttf", 40);
    if (!pGame->pFont)
    {
        printf("ERROR TTF_OpenFont: %s\n", TTF_GetError());
        close(pGame);
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

    if (!(pGame->udpSocket = SDLNet_UDP_Open(0)))
    {
        printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return 0;
    }
    if (SDLNet_ResolveHost(&(pGame->serverAddress), "127.0.0.1", 2000))
    {
        printf("SDLNet_ResolveHost(127.0.0.1 2000): %s\n", SDLNet_GetError());
		return 0;
    }
    if (!(pGame->packet = SDLNet_AllocPacket(512)))
    {
        printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return 0;
    }
    pGame->packet->address.host = pGame->serverAddress.host;
    pGame->packet->address.port = pGame->serverAddress.port;
    

    pGame->pJoinText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "Press space to join server", 750,WINDOW_HEIGHT-75);
    if (!pGame->pJoinText)
    {
        printf("Erorr creating text: %s\n", SDL_GetError());
        close(pGame);
        return 0;
    }
    pGame->pWaitingText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont,  "Waiting for server...", 750,WINDOW_HEIGHT-75);
    if (!pGame->pWaitingText)
    {
        printf("WainErorr creating text: %s\n", SDL_GetError());
        close(pGame);
        return 0;
    }
    
    initObstacles(obstacles, NUM_OBSTACLES);
    initializePlayers(pGame);

    // Set initial game state
    pGame->state = GAME_START;
    return 1;
}

void run(Game *pGame) {
    bool running = true;
    SDL_Event e;
    ClientData cData;
    int joining = 0;

    while (running) {

        switch (pGame->state)
        {
        case GAME_ONGOING:
            while (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet))
            {
                updateWithServerData(pGame);
            }
            
            if (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                    running = false;
                else
                    handlePlayerInput(pGame, &e);
            }
            SDL_RenderClear(pGame->pRenderer);   
            SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL);
            drawObstacles(pGame->pRenderer, obstacles, NUM_OBSTACLES); //debug
            renderPlayers(pGame); // Draw all players
            SDL_RenderPresent(pGame->pRenderer);
            break;
        
        case GAME_OVER:
            /* code */
            break;
        case GAME_START:
            if(!joining){
                SDL_RenderClear(pGame->pRenderer);
                SDL_SetRenderDrawColor(pGame->pRenderer,50,50,50,200);
                drawText(pGame->pJoinText);
            }else
            {
                SDL_RenderClear(pGame->pRenderer);
                SDL_SetRenderDrawColor(pGame->pRenderer,50,50,50,200);
                drawText(pGame->pWaitingText);
            }
            SDL_RenderPresent(pGame->pRenderer);
            if(SDL_PollEvent(&e)){
                if (e.type == SDL_QUIT) running = false;
                else if (!joining && e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    joining = 1;
                    cData.command = CMD_READY;
                    cData.playerNumber = -1;
                    memcpy(pGame->packet->data, &cData, sizeof(ClientData));
                    pGame->packet->len = sizeof(ClientData);
                }
            }
            if (joining) SDLNet_UDP_Send(pGame->udpSocket,-1,pGame->packet);
            if (SDLNet_UDP_Recv(pGame->udpSocket,pGame->packet))
            {
                updateWithServerData(pGame);
                if(pGame->state == GAME_ONGOING) joining =false;
            }
            break;
        }
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
        //printf("Clip %d: x=%d, y=%d, w=%d, h=%d\n", i, player->spriteClips[i].x, player->spriteClips[i].y, player->spriteClips[i].w, player->spriteClips[i].h);
    }
}

void renderPlayers(Game *pGame) {
    for (int i = 0; i < 2; i++) {
        renderPlayer(pGame->pRenderer, &pGame->players[i]);
    }
}

void handlePlayerInput(Game *pGame, SDL_Event *pEvent) {
    int deltaX = 0, deltaY = 0;
    if (pEvent->type == SDL_KEYDOWN)
    {
        ClientData cData;
        cData.playerNumber = pGame->playerNr;
        switch (pEvent->key.keysym.scancode)
        {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            deltaY -= 8;
            cData.command = CMD_UP; 
            break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            deltaY += 8;
            cData.command = CMD_DOWN; 
            break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            deltaX -= 8;
            cData.command = CMD_LEFT; 
            break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            deltaX += 8;
            cData.command = CMD_RIGHT; 
            break;
        }
        moveCharacter(&pGame->players[pGame->playerNr].position, deltaX, deltaY, pGame->players[pGame->playerNr].type, obstacles, NUM_OBSTACLES);
        updateFrame(&pGame->players[pGame->playerNr].currentFrame, pGame->players[pGame->playerNr].type, 2, 3);
        memcpy(pGame->packet->data, &cData, sizeof(ClientData));
        pGame->packet->len = sizeof(ClientData);
        SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
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

void updateWithServerData(Game *pGame) {
    ServerData sData;
    memcpy(&sData, pGame->packet->data, sizeof(ServerData));
    pGame->playerNr = sData.playerNr;
    pGame->state = sData.state;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->players[i].position.x = sData.players[i].x;
        pGame->players[i].position.y = sData.players[i].y;

        if (sData.players[i].role == ROLE_HUNTER) {
            pGame->players[i].type = HUNTER;
            pGame->players[i].texture = pGame->hunterTexture;
            printf("Player %d assigned HUNTER texture.\n", i);
        } else if (sData.players[i].role == ROLE_SPRINTER) {
            pGame->players[i].type = SPRINTER;
            pGame->players[i].texture = pGame->sprinterTexture;
            printf("Player %d assigned SPRINTER texture.\n", i);
        }
    }
}



void initializePlayers(Game *pGame) {
    int sprinterIndex = 0;

    hunter = createHunterMan(600, 300);
    pGame->players[0].isActive = 1;
    pGame->players[0].texture = pGame->hunterTexture;
    pGame->players[0].position = (SDL_Rect){getHunterPositionX(hunter), getHunterPositionY(hunter), 32, 32};
    pGame->players[0].type = HUNTER;
    setupPlayerClips(&pGame->players[0]);

    for (int i = 1; i < MAX_PLAYERS; i++) {
        SDL_Point spawn = sprinterSpawnPoints[sprinterIndex];
        sprinters[sprinterIndex] = createSprinterMan(spawn.x, spawn.y);

        pGame->players[i].isActive = 1;
        pGame->players[i].texture = pGame->sprinterTexture;
        pGame->players[i].position = (SDL_Rect){getSprinterPositionX(sprinters[sprinterIndex]), getSprinterPositionY(sprinters[sprinterIndex]), 32, 32};
        pGame->players[i].type = SPRINTER;
        setupPlayerClips(&pGame->players[i]);

        sprinterIndex++;
    }
}