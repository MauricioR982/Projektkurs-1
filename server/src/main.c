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
    SDL_Texture *backgroundTexture, *hunterTexture, *sprinterTexture, *initialTextTexture;
    
    IPaddress serverAddress;
    GameState state;
    TTF_Font *pFont;
    Text *pWaitingText;

    UDPsocket udpSocket;
    UDPpacket *packet;
    IPaddress clients[MAX_PLAYERS];
    int nrOfClients;
    ServerData sData;
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
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles);
void updateFrame(int *frame, PlayerRole role, int frame1, int frame2);
bool checkCollision(SDL_Rect a, SDL_Rect b);
void updateWithServerData(Game *pGAme);
void add(IPaddress address, IPaddress client[] , int *pNrOfClents);
void setUpGame(Game *pGame);
void sendGameData(Game *pGame);
void executeCommand(Game *pGame, ClientData cData);
void renderPlayer(SDL_Renderer *renderer, Player *player);
void initializePlayers(Game *pGame);
void swapHunterAndSprinter(Player *hunter, Player *sprinter, SDL_Texture *hunterTexture, SDL_Texture *sprinterTexture);

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
    pGame->pWindow = SDL_CreateWindow("Game Server", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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

    if (!(pGame->udpSocket = SDLNet_UDP_Open(2000)))
    {
        printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		close(pGame);
        return 0;
    }
    if (!(pGame->packet = SDLNet_AllocPacket(512)))
    {
        printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		close(pGame);
        return 0;
    }
    
    pGame->pWaitingText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont,  "Waiting for clients...", 750,WINDOW_HEIGHT-75);
    if (!pGame->pWaitingText)
    {
        printf("Waiting Error creating text: %s\n", SDL_GetError());
        close(pGame);
        return 0;
    }

    initObstacles(obstacles, NUM_OBSTACLES);
    initializePlayers(pGame);

    // Set initial game state
    pGame->state = GAME_START;
    pGame->nrOfClients =0;
    return 1;
}

void run(Game *pGame) {
    bool running = true, joining = false;
    SDL_Event e;
    ClientData cData;

    while (running) {

        switch (pGame->state)
        {
        case GAME_ONGOING:
            sendGameData(pGame);
            while (SDLNet_UDP_Recv(pGame->udpSocket,pGame->packet) == 1)
            {
                memcpy(&cData, pGame->packet->data,sizeof(ClientData));
                executeCommand(pGame,cData);
            }
            if (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = false;
            SDL_RenderClear(pGame->pRenderer);          
            SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL);
            drawObstacles(pGame->pRenderer, obstacles, NUM_OBSTACLES);
            renderPlayers(pGame); // Draw all players
            SDL_RenderPresent(pGame->pRenderer);
        
            
            break;
        
        case GAME_OVER:
            /* code */
            break;
        case GAME_START:
            SDL_RenderClear(pGame->pRenderer);
            SDL_SetRenderDrawColor(pGame->pRenderer,50,50,50,200);
            drawText(pGame->pWaitingText);
            SDL_RenderPresent(pGame->pRenderer);
            if(SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = false;
            if (SDLNet_UDP_Recv(pGame->udpSocket,pGame->packet)==1)
            {

                add(pGame->packet->address, pGame->clients, &(pGame->nrOfClients));
                if(pGame->nrOfClients == MAX_PLAYERS) setUpGame(pGame);
            }
            break;
        }
    }
}

void setUpGame(Game *pGame){
    pGame->state = GAME_ONGOING;
}

/*void sendGameData(Game *pGame){
    pGame->sData.state = pGame->state;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        pGame->sData.players[i].x = pGame->players[i].position.x;
        pGame->sData.players[i].y = pGame->players[i].position.y;
    }

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        pGame->sData.playerNr = i;
        memcpy(pGame->packet->data, &(pGame->sData), sizeof(ServerData));
        pGame->packet->len = sizeof(ServerData);
        pGame->packet->address = pGame->clients[i];
        SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
    }    
}*/

void sendGameData(Game *pGame) {
    pGame->sData.state = pGame->state;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->sData.players[i].x = pGame->players[i].position.x;
        pGame->sData.players[i].y = pGame->players[i].position.y;

        // Ensure that roles are set correctly
        if (pGame->players[i].type == HUNTER) {
            pGame->sData.players[i].role = ROLE_HUNTER;
        } else {
            pGame->sData.players[i].role = ROLE_SPRINTER;
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->sData.playerNr = i;
        memcpy(pGame->packet->data, &(pGame->sData), sizeof(ServerData));
        pGame->packet->len = sizeof(ServerData);
        pGame->packet->address = pGame->clients[i];
        SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
    }
}


void add(IPaddress address, IPaddress client[] , int *pNrOfClents){
    //printf("Adding player\n");

    if ((*pNrOfClents) >= MAX_PLAYERS)
    {
        printf("Abort adding player\n");
    }
    

    for (size_t i = 0; i < (*pNrOfClents); i++)
    {
        if (client[i].host == address.host && client[i].port == address.port)
        {
            printf("Abort adding player\n");
            return;
        }
    }
    
    client[(*pNrOfClents)] = address;
    printf("\n\n%d\n\n",address.host);
    printf("\n\n%d\n\n",address.port);
    (*pNrOfClents)++;
    printf("\n\nnrOfClients: %d\n\n",(*pNrOfClents));
    printf("Player added successfully\n");
    
}

void executeCommand(Game *pGame, ClientData cData) {
    int deltaX = 0, deltaY = 0;
    switch (cData.command) {
        case CMD_UP:
            deltaY -= 8;
            break;
        case CMD_DOWN:
            deltaY += 8;
            break;
        case CMD_LEFT:
            deltaX -= 8;
            break;
        case CMD_RIGHT:
            deltaX += 8;
            break;
    }

    // Move the player according to input command
    moveCharacter(&pGame->players[cData.playerNumber].position, deltaX, deltaY, pGame->players[cData.playerNumber].type, obstacles, NUM_OBSTACLES);
    updateFrame(&pGame->players[cData.playerNumber].currentFrame, pGame->players[cData.playerNumber].type, 2, 3);

    // If the player is a hunter, check for collisions with sprinters
    if (pGame->players[cData.playerNumber].type == HUNTER) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            // Ignore self and other hunters
            if (i == cData.playerNumber || pGame->players[i].type == HUNTER) continue;

            // If a collision occurs between the hunter and a sprinter
            if (checkCollision(pGame->players[cData.playerNumber].position, pGame->players[i].position)) {
<<<<<<< HEAD
                // Switch the sprinter to a hunter
=======
                // Öka poängen för jägaren
                pGame->players[cData.playerNumber].score += POINTS_PER_TAG;

                // Byt sprinter till en jägare
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> parent of 686907c (score code added)
=======
>>>>>>> parent of 686907c (score code added)
=======
>>>>>>> parent of 686907c (score code added)
                pGame->players[i].type = HUNTER;
                pGame->players[i].texture = pGame->hunterTexture;
                pGame->players[i].currentFrame = 0;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                // Change the previous hunter to a sprinter
=======
                // Ändra den tidigare jägaren till en sprinter
>>>>>>> parent of 686907c (score code added)
=======
                // Ändra den tidigare jägaren till en sprinter
>>>>>>> parent of 686907c (score code added)
=======
                // Ändra den tidigare jägaren till en sprinter
>>>>>>> parent of 686907c (score code added)
                pGame->players[cData.playerNumber].type = SPRINTER;
                pGame->players[cData.playerNumber].texture = pGame->sprinterTexture;
                pGame->players[cData.playerNumber].currentFrame = 0;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                // You can add extra logic here (e.g., notifications or score updates)

                break; // Exit loop after one successful swap
=======
=======
>>>>>>> parent of 686907c (score code added)
=======
>>>>>>> parent of 686907c (score code added)
                // Avbryt loopen efter en framgångsrik byte
                break;
>>>>>>> parent of 686907c (score code added)
            }
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

void swapHunterAndSprinter(Player *hunter, Player *sprinter, SDL_Texture *hunterTexture, SDL_Texture *sprinterTexture) {
    // Swap positions
    SDL_Rect tempPos = hunter->position;
    hunter->position = sprinter->position;
    sprinter->position = tempPos;

    // Swap roles and textures
    hunter->type = SPRINTER;
    hunter->texture = sprinterTexture;
    sprinter->type = HUNTER;
    sprinter->texture = hunterTexture;

    // Reset to frame 0
    hunter->currentFrame = 0;
    sprinter->currentFrame = 0;
}
