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

    Uint32 startTime;
    int gameDuration;
    SDL_Texture *speedPerkTexture;   // Texture för SPEED perk
    SDL_Texture *stuckPerkTexture;   // Texture för STUCK perk
    Perk perks[MAX_PERKS];
    int numPerks;
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
void setupPlayerClips(SDL_Rect spriteClips[]);
void renderPlayers(Game *pGame);
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, float speed, Obstacle obstacles[], int numObstacles);
void updateFrame(int *frame, int frame1, int frame2);
bool checkCollision(SDL_Rect a, SDL_Rect b);
void updateWithServerData(Game *pGAme);
void add(IPaddress address, IPaddress client[] , int *pnrOfClients);
void setUpGame(Game *pGame);
void sendGameData(Game *pGame);
void executeCommand(Game *pGame, ClientData cData);
void renderPlayer(SDL_Renderer *renderer, Player *player);
void initializePlayers(Game *pGame);
void swapHunterAndSprinter(Player *hunter, Player *sprinter, SDL_Texture *hunterTexture, SDL_Texture *sprinterTexture);
void checkGameOverCondition(Game *pGame);
void createRandomPerk(Game *pGame, int index);
void initiatePerks(Game *pGame);
void applyPerk(Game *pGame, Player *player, Perk *perk);
void updatePerkMovement(Game *pGame, int deltaMs);
void renderPerks(Game *pGame);
void resetPerk(Player *player);

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
    initiatePerks(pGame);

    // Set initial game state
    pGame->state = GAME_START;
    pGame->nrOfClients =0;

    // Initialize timer in the `initiate` function:
    pGame->startTime = SDL_GetTicks();  // Record the start time
    pGame->gameDuration = 60000;        // 1 minute in milliseconds

    return 1;
}

void initiatePerks(Game *pGame) {
    for (int i = 0; i < MAX_PERKS; i++) {
        pGame->perks[i].active = false;
        pGame->perks[i].perkSpawnTimer = 0;
        pGame->perks[i].perkSpawnInterval = 8000; // 8 sekunder
    }
    pGame->numPerks = 0;
}

void createRandomPerk(Game *pGame, int index) {
    if (pGame->numPerks >= MAX_PERKS) return;

    int type = rand() % 2; // Väljer mellan SPEED och STUCK
    SDL_Rect position = {rand() % (WINDOW_WIDTH - 30), rand() % (WINDOW_HEIGHT - 30), 30, 30};
    int dx = (rand() % 3 - 1) * 2;  // Hastighet i x-led, -2, 0 eller 2
    int dy = (rand() % 3 - 1) * 2;  // Hastighet i y-led, -2, 0 eller 2

    pGame->perks[index] = (Perk){type, position, 5000, true, SDL_GetTicks()}; // 5 sekunder varaktighet
    pGame->numPerks++;
}

void run(Game *pGame) {
    bool running = true;
    SDL_Event e;
    ClientData cData;
    Uint32 lastUpdate = SDL_GetTicks();    

    while (running) {
        Uint32 currentUpdate = SDL_GetTicks();
        Uint32 deltaTime = currentUpdate - lastUpdate;
        lastUpdate = currentUpdate;

        // Hantera skapandet av nya perks
        for (int i = 0; i < MAX_PERKS; i++) {
            if (!pGame->perks[i].active) {
                pGame->perks[i].perkSpawnTimer += deltaTime;
                if (pGame->perks[i].perkSpawnTimer >= pGame->perks[i].perkSpawnInterval) {
                    createRandomPerk(pGame, i);
                    pGame->perks[i].perkSpawnTimer = 0;
                }
            }
        }

        // Hantera SDL-events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        switch (pGame->state) {
            case GAME_ONGOING:
                sendGameData(pGame);
                while (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet) == 1) {
                    memcpy(&cData, pGame->packet->data, sizeof(ClientData));
                    executeCommand(pGame, cData);
                }

                // Kontrollera om perks har gått ut
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (pGame->players[i].activePerkType != -1) { // Om spelaren har en aktiv perk
                        Uint32 perkDuration = SDL_GetTicks() - pGame->players[i].perkStartTime;
                        if (perkDuration >= 5000) { // 5 sekunders varaktighet
                            resetPerk(&pGame->players[i]);
                        }
                    }
                }

                SDL_RenderClear(pGame->pRenderer);
                SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL);
                drawObstacles(pGame->pRenderer, obstacles, NUM_OBSTACLES);
                updatePerkMovement(pGame, 80);  // 'deltaMs' är tiden sedan senaste frame/uppdatering
                renderPlayers(pGame);
                renderPerks(pGame);
                SDL_RenderPresent(pGame->pRenderer);
                break;

            case GAME_OVER:
                // Continue sending data to clients about the game-over state
                sendGameData(pGame);

                SDL_RenderClear(pGame->pRenderer);
                // Optionally render a simple "Game Over" message or screen here
                // (or simply clear the screen)

                SDL_RenderPresent(pGame->pRenderer);

                // Keep checking for client events to maintain responsiveness
                while (running) {
                    if (SDL_PollEvent(&e)) {
                        if (e.type == SDL_QUIT) {
                            running = false;
                        }
                    }
                    SDL_Delay(100);  // Prevents high CPU usage in this loop
                }
                break;

            case GAME_START:
                SDL_RenderClear(pGame->pRenderer);
                SDL_SetRenderDrawColor(pGame->pRenderer, 50, 50, 50, 200);
                drawText(pGame->pWaitingText);
                SDL_RenderPresent(pGame->pRenderer);
                if (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet) == 1) {
                    add(pGame->packet->address, pGame->clients, &(pGame->nrOfClients));
                    if (pGame->nrOfClients == MAX_PLAYERS) setUpGame(pGame);
                }
                break;
        }
    }
}

void resetPerk(Player *player) {
    player->speed = player->originalSpeed; // Återställ spelarens hastighet
    player->activePerkType = -1; // Ingen aktiv perk
}

void setUpGame(Game *pGame){
    pGame->state = GAME_ONGOING;
}

void sendGameData(Game *pGame) {
    Uint32 currentTime = SDL_GetTicks();
    int elapsedTime = currentTime - pGame->startTime;
    int remainingTime = (pGame->gameDuration - elapsedTime) / 1000;
    if (remainingTime < 0) remainingTime = 0;  // Prevent negative values

    // Add remaining time to the data packet
    pGame->sData.remainingTime = remainingTime;
    pGame->sData.state = pGame->state;  // Ensure current state is shared with clients

    // Copy player data to the packet
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->sData.players[i].x = pGame->players[i].position.x;
        pGame->sData.players[i].y = pGame->players[i].position.y;
        pGame->sData.players[i].currentFrame = pGame->players[i].currentFrame;
        pGame->sData.players[i].flip = pGame->players[i].flip;

        if (pGame->players[i].type == HUNTER) {
            pGame->sData.players[i].role = ROLE_HUNTER;
        } else {
            pGame->sData.players[i].role = ROLE_SPRINTER;
        }

     // Update perk data
    for (int i = 0; i < MAX_PERKS; i++) {
        pGame->sData.perks[i].type = pGame->perks[i].type;
        pGame->sData.perks[i].position = pGame->perks[i].position;
        pGame->sData.perks[i].active = pGame->perks[i].active;
    }

    // Send data to each client
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->sData.playerNr = i;
        memcpy(pGame->packet->data, &(pGame->sData), sizeof(ServerData));
        pGame->packet->len = sizeof(ServerData);
        pGame->packet->address = pGame->clients[i];
        SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
    }
}
}

void add(IPaddress address, IPaddress client[] , int *pnrOfClients){
    //printf("Adding player\n");

    if ((*pnrOfClients) >= MAX_PLAYERS)
    {
        //printf("Abort adding player\n");
    }
    

    for (size_t i = 0; i < (*pnrOfClients); i++)
    {
        if (client[i].host == address.host && client[i].port == address.port)
        {
            //printf("Abort adding player\n");
            return;
        }
    }
    
    client[(*pnrOfClients)] = address;
    printf("\n\n%d\n\n",address.host);
    printf("\n\n%d\n\n",address.port);
    (*pnrOfClients)++;
    printf("\n\nnrOfClients: %d\n\n",(*pnrOfClients));
    printf("Player added successfully\n");
    
}

void executeCommand(Game *pGame, ClientData cData) {
    int deltaX = 0, deltaY = 0;
    int frame1 = 0, frame2 = 1; // Default frame values
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    switch (cData.command) {
         case CMD_UP:
            deltaY -= 8;
            frame1 = 4; // Facing up
            frame2 = 5;
            break;
        case CMD_DOWN:
            deltaY += 8;
            frame1 = 0; // Facing down
            frame2 = 1;
            break;
        case CMD_LEFT:
            deltaX -= 8;
            frame1 = 2; // Right-facing sprites
            frame2 = 3;
            flip = SDL_FLIP_HORIZONTAL; // Apply horizontal flip
            break;
        case CMD_RIGHT:
            deltaX += 8;
            frame1 = 2; // Right-facing sprites
            frame2 = 3;
            flip = SDL_FLIP_NONE; // No flip
            break;
        case CMD_RESET:
            pGame->state = GAME_START;
            pGame->nrOfClients = 0;
            pGame->startTime = SDL_GetTicks(); // Reset the timer
            return;
    }

    // Flytta spelaren enligt kommando och hastighet
    moveCharacter(&pGame->players[cData.playerNumber].position, deltaX, deltaY, pGame->players[cData.playerNumber].speed, obstacles, NUM_OBSTACLES);
    updateFrame(&pGame->players[cData.playerNumber].currentFrame, frame1, frame2);
    pGame->players[cData.playerNumber].flip = flip; // Set the flip state

    checkGameOverCondition(pGame);

    // Kontrollera kollision med perks
    for (int i = 0; i < pGame->numPerks; i++) {
        if (pGame->perks[i].active && checkCollision(pGame->players[cData.playerNumber].position, pGame->perks[i].position)) {
            applyPerk(pGame, &pGame->players[cData.playerNumber], &pGame->perks[i]);
            pGame->perks[i].active = false; // Gör perken inaktiv efter aktivering
        }
    }

    // Om spelaren är en jägare, kontrollera kollisioner med sprinters
    if (pGame->players[cData.playerNumber].type == HUNTER) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (i != cData.playerNumber && pGame->players[i].type != HUNTER) {
                if (checkCollision(pGame->players[cData.playerNumber].position, pGame->players[i].position)) {
                    pGame->players[i].type = HUNTER;
                    pGame->players[i].texture = pGame->hunterTexture;
                    pGame->players[i].currentFrame = 0;

                    pGame->players[cData.playerNumber].type = SPRINTER;
                    pGame->players[cData.playerNumber].texture = pGame->sprinterTexture;
                    pGame->players[cData.playerNumber].currentFrame = 0;

                    break; // Avsluta loopen efter en lyckad byte
                }
            }
        }
    }
}

void applyPerk(Game *pGame, Player *player, Perk *perk) {
    switch (perk->type) {
        case 0: // SPEED
            player->speed = 3 * player->originalSpeed;
            break;
        case 1: // STUCK
            player->speed = player->originalSpeed / 3;
            break;
    }
    perk->startTime = SDL_GetTicks(); // Starta tid för perkens varaktighet
    perk->active = true;
    player->perkStartTime = SDL_GetTicks(); // Lägg till detta fält i Player-strukturen
    player->activePerkType = perk->type; // Spara typen av aktiv perk
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

    SDL_Surface *surface;

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

    surface = IMG_Load("../lib/resources/SPEED.png");
    if (!surface) {
        fprintf(stderr, "Failed to load SPEED perk image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->speedPerkTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("../lib/resources/STUCK.png");
    if (!surface) {
        fprintf(stderr, "Failed to load STUCK perk image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->stuckPerkTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // Se till att alla texturer skapades framgångsrikt
    if (!pGame->speedPerkTexture || !pGame->stuckPerkTexture) {
        SDL_DestroyTexture(pGame->speedPerkTexture);
        SDL_DestroyTexture(pGame->stuckPerkTexture);
        return 0;
    }

    return 1;
}

void updatePerkMovement(Game *pGame, int deltaMs) {
    float speedFactor = 0.012;  // Minska denna faktor för långsammare rörelse
    for (int i = 0; i < pGame->numPerks; i++)
    {
        if (pGame->perks[i].active)
        {
            pGame->perks[i].position.x += (int)(pGame->perks[i].dx * speedFactor * (deltaMs / 1000.0));
            pGame->perks[i].position.y += (int)(pGame->perks[i].dy * speedFactor * (deltaMs / 1000.0));
            if (pGame->perks[i].position.x < 0 || pGame->perks[i].position.x + pGame->perks[i].position.w > WINDOW_WIDTH)
            {
                pGame->perks[i].dx = -pGame->perks[i].dx;
                pGame->perks[i].position.x += (int)(pGame->perks[i].dx * speedFactor * (deltaMs / 1000.0));
            }
            if (pGame->perks[i].position.y < 0 || pGame->perks[i].position.y + pGame->perks[i].position.h > WINDOW_HEIGHT)
            {
                pGame->perks[i].dy = -pGame->perks[i].dy;
                pGame->perks[i].position.y += (int)(pGame->perks[i].dy * speedFactor * (deltaMs / 1000.0));
            }
            pGame->perks[i].perkSpawnTimer += deltaMs;
            if (pGame->perks[i].perkSpawnTimer >= 5000)
            {
                pGame->perks[i].position.x = rand() % (WINDOW_WIDTH - pGame->perks[i].position.w);
                pGame->perks[i].position.y = rand() % (WINDOW_HEIGHT - pGame->perks[i].position.h);
                pGame->perks[i].perkSpawnTimer = 0;
            }
        }
    }
}

void renderPerks(Game *pGame) {
    for (int i = 0; i < pGame->numPerks; i++) {
        if (pGame->perks[i].active) {
            SDL_Texture* texture = (pGame->perks[i].type == 0) ? pGame->speedPerkTexture : pGame->stuckPerkTexture;
            SDL_RenderCopyEx(pGame->pRenderer, texture, NULL, &pGame->perks[i].position, 0, NULL, SDL_FLIP_NONE);
        }
    }
}

void renderPlayer(SDL_Renderer *renderer, Player *player) {
    if (!player->isActive) return;
    SDL_Rect srcRect = player->spriteClips[player->currentFrame];
    SDL_Rect destRect = {player->position.x, player->position.y, player->position.w, player->position.h};
    SDL_RenderCopyEx(renderer, player->texture, &srcRect, &destRect, 0, NULL, player->flip);
}

void setupPlayerClips(SDL_Rect spriteClips[]) {
    // Down
    spriteClips[0] = (SDL_Rect){0, 0, 16, 16};
    spriteClips[1] = (SDL_Rect){16, 0, 16, 16};
    // Up
    spriteClips[2] = (SDL_Rect){32, 0, 16, 16};
    spriteClips[3] = (SDL_Rect){48, 0, 16, 16};
    // Left
    spriteClips[4] = (SDL_Rect){64, 0, 16, 16};
    spriteClips[5] = (SDL_Rect){80, 0, 16, 16};
    // Right
    spriteClips[6] = (SDL_Rect){96, 0, 16, 16};
    spriteClips[7] = (SDL_Rect){112, 0, 16, 16};
}

void renderPlayers(Game *pGame) {
    for (int i = 0; i < 2; i++) {
        renderPlayer(pGame->pRenderer, &pGame->players[i]);
    }
}
// Function to move character with collision checking
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, float speed, Obstacle obstacles[], int numObstacles) {
    SDL_Rect newPos = {charPos->x + deltaX * speed, charPos->y + deltaY * speed, charPos->w, charPos->h};

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

void updateFrame(int *frame, int frame1, int frame2) {
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
    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->players[i].speed = 1.0; // Standardhastighet
        pGame->players[i].originalSpeed = 1.0; // Spara den ursprungliga hastigheten
    }

    int sprinterIndex = 0;
    hunter = createHunterMan(600, 300);
    pGame->players[0].isActive = 1;
    pGame->players[0].texture = pGame->hunterTexture;
    pGame->players[0].position = (SDL_Rect){getHunterPositionX(hunter), getHunterPositionY(hunter), 32, 32};
    pGame->players[0].type = HUNTER;
    setupPlayerClips(pGame->players[0].spriteClips);  // Använd setupPlayerClips

    for (int i = 1; i < MAX_PLAYERS; i++) {
        SDL_Point spawn = sprinterSpawnPoints[sprinterIndex];
        sprinters[sprinterIndex] = createSprinterMan(spawn.x, spawn.y);

        pGame->players[i].isActive = 1;
        pGame->players[i].texture = pGame->sprinterTexture;
        pGame->players[i].position = (SDL_Rect){getSprinterPositionX(sprinters[sprinterIndex]), getSprinterPositionY(sprinters[sprinterIndex]), 32, 32};
        pGame->players[i].type = SPRINTER;
        setupPlayerClips(pGame->players[i].spriteClips);  // Använd setupPlayerClips

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

void checkGameOverCondition(Game *pGame) {
    Uint32 currentTime = SDL_GetTicks();
    int elapsedTime = currentTime - pGame->startTime;
    int remainingTime = (pGame->gameDuration - elapsedTime) / 1000;

    if (remainingTime <= 0) {
        pGame->state = GAME_OVER;  // Set game state to GAME_OVER
        // Optionally, add further game-over logic here
    }
}