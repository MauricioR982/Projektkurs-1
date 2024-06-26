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
    Text *pStartText;
    Text *pTutorialText;
    Text *pExitText;
    int selectedItem;
} Menu;

typedef struct {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    Player players[MAX_PLAYERS];
    int playerNr;
    SDL_Texture *backgroundTexture, *hunterTexture, *sprinterTexture, *initialTextTexture, *tutorialTexture, *menuBackgroundTexture, *gameOverHunterTexture, *gameOverSprinterTexture;
    UDPsocket udpSocket;
    UDPpacket *packet;
    IPaddress serverAddress;
    GameState state;
    TTF_Font *pFont;
    Text *pWaitingText, *pJoinText;
    Menu menu;
    Uint32 startTime;    
    int gameDuration; 
    Text *pTimerText, *pResetText;
    SDL_Texture *speedPerkTexture;
    SDL_Texture *stuckPerkTexture;
    Perk perks[MAX_PERKS];
    int numPerks;
} Game;

Obstacle obstacles[NUM_OBSTACLES];
Hunter hunter;  // One hunter
Sprinter sprinters[MAX_PLAYERS - 1]; //Remaining players become sprinters

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
int loadGameResources(SDL_Renderer *renderer, Game *pGame);
void renderPlayer(SDL_Renderer *renderer, Player *player);
void setupPlayerClips(Player *player);
void renderPlayers(Game *pGame);
void handlePlayerInput(Game *pGame, SDL_Event *pEvent);
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles);
void updateFrame(int *frame, int frame1, int frame2);
bool checkCollision(SDL_Rect a, SDL_Rect b);
void updateWithServerData(Game *pGame);
void initializePlayers(Game *pGame);
int initiateMenu(Game *pGame);
void renderMenu(Game *pGame);
void handleTextInput(SDL_Event *e, char *inputText, int maxLength);
void renderPerks(Game *pGame);
void initiatePerks(Game *pGame);
void createFixedPerk(Game *pGame, int index, int type, int x, int y);

int main(int argc, char **argv) {
    Game g = {0};
    if (!initiate(&g)) return 1;
    run(&g);
    close(&g);
    return 0;
}

int initiate(Game *pGame) {
    
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
    
    if (!loadGameResources(pGame->pRenderer, pGame)) {
        SDL_DestroyRenderer(pGame->pRenderer);
        SDL_DestroyWindow(pGame->pWindow);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    if (!(pGame->udpSocket = SDLNet_UDP_Open(0))) // Open a socket on any available port
    {
        printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return 0;
    }
    if (!(pGame->packet = SDLNet_AllocPacket(512)))
    {
        printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return 0;
    }
    

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

    
    if (!initiateMenu(pGame)) {
        printf("Error creating menu items.\n");
        close(pGame);
        return 0;
    }

    pGame->pResetText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "Press Enter to play again!", 640, 650);
    if (!pGame->pResetText) {
        printf("Error creating reset text: %s\n", SDL_GetError());
        close(pGame);
        return 0;
    }
    pGame->startTime = SDL_GetTicks();  // Record start time
    pGame->gameDuration = 90000;        

    pGame->pTimerText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "01:00", WINDOW_WIDTH / 2, 30);
    if (!pGame->pTimerText) {
        fprintf(stderr, "Error creating timer text: %s\n", SDL_GetError());
        close(pGame);
        return 0;
    }

    initiatePerks(pGame);

    pGame->state = GAME_MENU;
    return 1;
}

void run(Game *pGame) {
    bool running = true;
    SDL_Event e;
    ClientData cData;
    int joining = 0;
    char ipAddress[16] = ""; // Buffer to store the IP address
    bool validConnection = false; // Flag to check if connection is valid

    while (running) {
        switch (pGame->state) {
            case GAME_MENU:
                renderMenu(pGame);
                while (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) {
                        running = false;
                    } else if (e.type == SDL_KEYDOWN) {
                        switch (e.key.keysym.scancode) {
                            case SDL_SCANCODE_UP:
                                pGame->menu.selectedItem = (pGame->menu.selectedItem + 2) % 3;
                                break;
                            case SDL_SCANCODE_DOWN:
                                pGame->menu.selectedItem = (pGame->menu.selectedItem + 1) % 3;
                                break;
                            case SDL_SCANCODE_RETURN:
                                if (pGame->menu.selectedItem == 0) {
                                    pGame->state = GAME_ENTER_IP; 
                                    SDL_StartTextInput();
                                } else if (pGame->menu.selectedItem == 1) {
                                    pGame->state = GAME_TUTORIAL; 
                                } else if (pGame->menu.selectedItem == 2) {
                                    running = false; 
                                }
                                break;
                        }
                    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                        
                        int mouseX = e.button.x;
                        int mouseY = e.button.y;

                       
                        if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &pGame->menu.pStartText->rect)) {
                            pGame->state = GAME_ENTER_IP; 
                            SDL_StartTextInput();
                        } else if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &pGame->menu.pTutorialText->rect)) {
                            pGame->state = GAME_TUTORIAL; 
                        } else if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &pGame->menu.pExitText->rect)) {
                            running = false; 
                        }
                    }
                }
                break;

            case GAME_ENTER_IP:
                SDL_RenderClear(pGame->pRenderer);

                if (pGame->menuBackgroundTexture) {
                    SDL_RenderCopy(pGame->pRenderer, pGame->menuBackgroundTexture, NULL, NULL);
                }

                Text *pIpPromptText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "Enter Server IP:", 640, 200);
                if (!pIpPromptText) {
                    printf("Error: Failed to create IP prompt text\n");
                }

                
                Text *pIpInputText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, *ipAddress ? ipAddress : " ", 640, 300);
                if (!pIpInputText) {
                    printf("Error: Failed to create IP input text\n");
                }

                if (pIpPromptText && pIpInputText) {
                    drawText(pIpPromptText);
                    drawText(pIpInputText);
                    destroyText(pIpPromptText);
                    destroyText(pIpInputText);
                }
                SDL_RenderPresent(pGame->pRenderer);

                while (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) {
                        running = false;
                    } else {
                        handleTextInput(&e, ipAddress, 15);
                        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
                            SDL_StopTextInput();
                            printf("Entered IP Address: %s\n", ipAddress);
                            if (SDLNet_ResolveHost(&(pGame->serverAddress), ipAddress, 2000) == -1) {
                                printf("SDLNet_ResolveHost(%s 2000): %s\n", ipAddress, SDLNet_GetError());
                                validConnection = false;
                            } else {
                                validConnection = true;
                                pGame->packet->address.host = pGame->serverAddress.host;
                                pGame->packet->address.port = pGame->serverAddress.port;
                                pGame->state = GAME_START;
                            }
                        }
                    }
                }
                break;

            case GAME_TUTORIAL:
                SDL_RenderClear(pGame->pRenderer);
                if (pGame->tutorialTexture) {
                    SDL_RenderCopy(pGame->pRenderer, pGame->tutorialTexture, NULL, NULL);
                }
                SDL_RenderPresent(pGame->pRenderer);
                while (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) {
                        running = false;
                    } else if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        pGame->state = GAME_MENU; // Return to the menu
                    }
                }
                break;

            case GAME_ONGOING:
                while (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet)) {
                    updateWithServerData(pGame);
                }

                if (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) {
                        running = false;
                    } else {
                        handlePlayerInput(pGame, &e);
                    }
                }

                // Timer logic
                Uint32 currentTime = SDL_GetTicks();
                int elapsedTime = currentTime - pGame->startTime;
                int remainingTime = (pGame->gameDuration - elapsedTime) / 1000;
                if (remainingTime < 0) remainingTime = 0; // No negative values

                SDL_RenderClear(pGame->pRenderer);
                if (pGame->backgroundTexture) {
                    SDL_RenderCopy(pGame->pRenderer, pGame->backgroundTexture, NULL, NULL);
                }
                drawObstacles(pGame->pRenderer, obstacles, NUM_OBSTACLES); //debug
                renderPlayers(pGame); // Draw all players
                renderPerks(pGame);   // Render perks

                // Render the timer text in the upper-middle part of the screen
                if (pGame->pTimerText) {
                    drawText(pGame->pTimerText);
                }

                SDL_RenderPresent(pGame->pRenderer);
                break;

            case GAME_OVER:
                SDL_RenderClear(pGame->pRenderer);

                
                if (pGame->players[pGame->playerNr].type == HUNTER) {
                    if (pGame->gameOverHunterTexture) {
                        SDL_RenderCopy(pGame->pRenderer, pGame->gameOverHunterTexture, NULL, NULL);
                    }
                } else {
                    if (pGame->gameOverSprinterTexture) {
                        SDL_RenderCopy(pGame->pRenderer, pGame->gameOverSprinterTexture, NULL, NULL);
                    }
                }

                // Display "Press Enter to play again!" text
                if (pGame->pResetText) {
                    drawText(pGame->pResetText);
                }

                SDL_RenderPresent(pGame->pRenderer);

                // Detect Enter key press
                if (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) {
                        running = false;
                    } else if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                        // Send the reset command to the server
                        ClientData cData;
                        cData.command = CMD_RESET;
                        cData.playerNumber = pGame->playerNr;
                        memcpy(pGame->packet->data, &cData, sizeof(ClientData));
                        pGame->packet->len = sizeof(ClientData);
                        SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
                    }
                }
                break;

            case GAME_START:
                if (!joining) {
                    SDL_RenderClear(pGame->pRenderer);
                    SDL_SetRenderDrawColor(pGame->pRenderer, 50, 50, 50, 200);
                    if (pGame->pJoinText) {
                        drawText(pGame->pJoinText);
                    }
                } else {
                    SDL_RenderClear(pGame->pRenderer);
                    SDL_SetRenderDrawColor(pGame->pRenderer, 50, 50, 50, 200);
                    if (pGame->pWaitingText) {
                        drawText(pGame->pWaitingText);
                    }
                }
                SDL_RenderPresent(pGame->pRenderer);

                if (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) running = false;
                    else if (!joining && e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                        joining = 1;
                        cData.command = CMD_READY;
                        cData.playerNumber = -1;
                        memcpy(pGame->packet->data, &cData, sizeof(ClientData));
                        pGame->packet->len = sizeof(ClientData);
                    }
                }
                if (joining) SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
                if (SDLNet_UDP_Recv(pGame->udpSocket, pGame->packet)) {
                    updateWithServerData(pGame);
                    if (pGame->state == GAME_ONGOING) joining = false;
                }
                break;
        }
    }
}

void close(Game *pGame) {
    if (pGame->pResetText) destroyText(pGame->pResetText);
    if (pGame->packet) SDLNet_FreePacket(pGame->packet);
    if (pGame->udpSocket) SDLNet_UDP_Close(pGame->udpSocket);
    SDLNet_Quit();
    if (pGame->hunterTexture) SDL_DestroyTexture(pGame->hunterTexture);
    if (pGame->sprinterTexture) SDL_DestroyTexture(pGame->sprinterTexture);
    if (pGame->backgroundTexture) SDL_DestroyTexture(pGame->backgroundTexture);
    if (pGame->speedPerkTexture) SDL_DestroyTexture(pGame->speedPerkTexture);
    if (pGame->stuckPerkTexture) SDL_DestroyTexture(pGame->stuckPerkTexture);
    if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
    if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);
    SDL_Quit();
}

void renderPerks(Game *pGame) {
    for (int i = 0; i < MAX_PERKS; i++) {
        if (pGame->perks[i].active) {
            SDL_Texture* texture = (pGame->perks[i].type == 0) ? pGame->speedPerkTexture : pGame->stuckPerkTexture;
            SDL_RenderCopyEx(pGame->pRenderer, texture, NULL, &pGame->perks[i].position, 0, NULL, SDL_FLIP_NONE);
        }
    }
}

int loadGameResources(SDL_Renderer *renderer, Game *pGame) {
    SDL_Surface *surface;

    SDL_Surface *menuSurface = IMG_Load("../lib/resources/MENU.png");
    if (!menuSurface) {
        fprintf(stderr, "Failed to load MENU background image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuSurface);
    SDL_FreeSurface(menuSurface);

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

     // Load tutorial image
    SDL_Surface *tutorialSurface = IMG_Load("../lib/resources/TUTORIAL.png");
    if (!tutorialSurface) {
        fprintf(stderr, "Failed to load tutorial image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->tutorialTexture = SDL_CreateTextureFromSurface(renderer, tutorialSurface);
    SDL_FreeSurface(tutorialSurface);

    
    SDL_Surface *gameOverHunterSurface = IMG_Load("../lib/resources/GAMEOVER.png");
    if (!gameOverHunterSurface) {
        fprintf(stderr, "Failed to load GAMEOVER image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->gameOverHunterTexture = SDL_CreateTextureFromSurface(renderer, gameOverHunterSurface);
    SDL_FreeSurface(gameOverHunterSurface);

    SDL_Surface *gameOverSprinterSurface = IMG_Load("../lib/resources/VICTORY.png");
    if (!gameOverSprinterSurface) {
        fprintf(stderr, "Failed to load VICTORY image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->gameOverSprinterTexture = SDL_CreateTextureFromSurface(renderer, gameOverSprinterSurface);
    SDL_FreeSurface(gameOverSprinterSurface);

    SDL_Surface *speedSurface = IMG_Load("../lib/resources/SPEED.png");
    if (!speedSurface) {
        fprintf(stderr, "Failed to load SPEED perk image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->speedPerkTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);
    SDL_FreeSurface(speedSurface);

    SDL_Surface *stuckSurface = IMG_Load("../lib/resources/STUCK.png");
    if (!stuckSurface) {
        fprintf(stderr, "Failed to load STUCK perk image: %s\n", IMG_GetError());
        return 0;
    }
    pGame->stuckPerkTexture = SDL_CreateTextureFromSurface(renderer, stuckSurface);
    SDL_FreeSurface(stuckSurface);

    if (!pGame->speedPerkTexture || !pGame->stuckPerkTexture) {
        SDL_DestroyTexture(pGame->speedPerkTexture);
        SDL_DestroyTexture(pGame->stuckPerkTexture);
        return 0;
    }

    return 1;
}

void renderPlayer(SDL_Renderer *renderer, Player *player) {
    if (!player->isActive) return;
    SDL_Rect srcRect = player->spriteClips[player->currentFrame];
    SDL_Rect destRect = {player->position.x, player->position.y, player->position.w, player->position.h};
    SDL_RenderCopyEx(renderer, player->texture, &srcRect, &destRect, 0, NULL, player->flip);
}

void setupPlayerClips(Player *player) {
    for (int i = 0; i < 8; i++) {
        player->spriteClips[i] = (SDL_Rect){i * 16, 0, 16, 16};
    }
}

void renderPlayers(Game *pGame) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        renderPlayer(pGame->pRenderer, &pGame->players[i]);
    }
}

void handlePlayerInput(Game *pGame, SDL_Event *pEvent) {
    int deltaX = 0, deltaY = 0;
    int frame1 = 0, frame2 = 1; 
    SDL_RendererFlip flip = SDL_FLIP_NONE;

    if (pEvent->type == SDL_KEYDOWN) {
        ClientData cData;
        cData.playerNumber = pGame->playerNr;

        switch (pEvent->key.keysym.scancode) {
            case SDL_SCANCODE_W:
            case SDL_SCANCODE_UP:
                deltaY -= 8;
                cData.command = CMD_UP;
                frame1 = 4; 
                frame2 = 5;
                break;
            case SDL_SCANCODE_S:
            case SDL_SCANCODE_DOWN:
                deltaY += 8;
                cData.command = CMD_DOWN;
                frame1 = 0; 
                frame2 = 1;
                break;
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                deltaX -= 8;
                cData.command = CMD_LEFT;
                frame1 = 2; 
                frame2 = 3;
                flip = SDL_FLIP_HORIZONTAL; 
                break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                deltaX += 8;
                cData.command = CMD_RIGHT;
                frame1 = 2; 
                frame2 = 3;
                flip = SDL_FLIP_NONE;
                break;
        }

        moveCharacter(&pGame->players[pGame->playerNr].position, deltaX, deltaY, pGame->players[pGame->playerNr].type, obstacles, NUM_OBSTACLES);
        updateFrame(&pGame->players[pGame->playerNr].currentFrame, frame1, frame2);
        pGame->players[pGame->playerNr].flip = flip;

        memcpy(pGame->packet->data, &cData, sizeof(ClientData));
        pGame->packet->len = sizeof(ClientData);
        SDLNet_UDP_Send(pGame->udpSocket, -1, pGame->packet);
    }
}

void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, int type, Obstacle obstacles[], int numObstacles) {
    SDL_Rect newPos = {charPos->x + deltaX, charPos->y + deltaY, charPos->w, charPos->h};

    for (int i = 0; i < numObstacles; i++) {
        if (checkCollision(newPos, obstacles[i].bounds)) {
            return;  
        }
    }
    newPos.x = SDL_clamp(newPos.x, HORIZONTAL_MARGIN, WINDOW_WIDTH - newPos.w - HORIZONTAL_MARGIN);
    newPos.y = SDL_clamp(newPos.y, 0, WINDOW_HEIGHT - newPos.h);
    *charPos = newPos;
}

void updateFrame(int *frame, int frame1, int frame2) {
    *frame = (*frame == frame1) ? frame2 : frame1;
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
    
    if (a.x + a.w <= b.x || b.x + b.w <= a.x ||
        a.y + a.h <= b.y || b.y + b.h <= a.y) {
        return false;
    }
    return true;
}

void updateWithServerData(Game *pGame) {
    ServerData sData;
    memcpy(&sData, pGame->packet->data, sizeof(ServerData));

    pGame->state = sData.state;
    pGame->playerNr = sData.playerNr;

    char timerStr[6];
    snprintf(timerStr, sizeof(timerStr), "%02d:%02d", sData.remainingTime / 60, sData.remainingTime % 60);
    updateText(pGame->pTimerText, pGame->pRenderer, timerStr);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        pGame->players[i].position.x = sData.players[i].x;
        pGame->players[i].position.y = sData.players[i].y;
        pGame->players[i].currentFrame = sData.players[i].currentFrame;
        pGame->players[i].flip = sData.players[i].flip;

        if (sData.players[i].role == ROLE_HUNTER) {
            pGame->players[i].type = HUNTER;
            pGame->players[i].texture = pGame->hunterTexture;
        } else if (sData.players[i].role == ROLE_SPRINTER) {
            pGame->players[i].type = SPRINTER;
            pGame->players[i].texture = pGame->sprinterTexture;
        }
    }
    for (int i = 0; i < MAX_PERKS; i++) {
        pGame->perks[i].active = sData.perks[i].active;
        pGame->perks[i].type = sData.perks[i].type;
        pGame->perks[i].position = sData.perks[i].position;
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

void renderMenu(Game *pGame) {
    SDL_RenderClear(pGame->pRenderer);

    SDL_RenderCopy(pGame->pRenderer, pGame->menuBackgroundTexture, NULL, NULL);

    drawText(pGame->menu.pStartText);
    drawText(pGame->menu.pTutorialText);
    drawText(pGame->menu.pExitText);

    SDL_RenderPresent(pGame->pRenderer);
}

int initiateMenu(Game *pGame) {
    pGame->menu.pStartText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "Start Game", 640, 200);
    pGame->menu.pTutorialText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "Tutorial", 640, 300);
    pGame->menu.pExitText = createText(pGame->pRenderer, 255, 255, 255, pGame->pFont, "Exit", 640, 400);
    pGame->menu.selectedItem = 0;

    if (!pGame->menu.pStartText || !pGame->menu.pTutorialText || !pGame->menu.pExitText) {
        return 0;
    }
    return 1;
}

void handleTextInput(SDL_Event *e, char *inputText, int maxLength) {
    if (e->type == SDL_TEXTINPUT) {
        if (strlen(inputText) + strlen(e->text.text) < maxLength) {
            strcat(inputText, e->text.text);
        }
    } else if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
            inputText[strlen(inputText) - 1] = '\0';
        }
    }
}

void initiatePerks(Game *pGame) {
    pGame->numPerks = MAX_PERKS;
    
    createFixedPerk(pGame, 0, 0, 200, 300); 
    createFixedPerk(pGame, 1, 0, 1080, 300); 
    createFixedPerk(pGame, 2, 1, 640, 50); 
    createFixedPerk(pGame, 3, 1, 640, 500); 
}

void createFixedPerk(Game *pGame, int index, int type, int x, int y) {
    pGame->perks[index].active = true;
    pGame->perks[index].type = type; 
    pGame->perks[index].position = (SDL_Rect){x, y, 32, 32};
}
