//
//  'main.c'
//  Developed by Grupp 10 - Datateknik, project-start at 2024-03.
//

//lägg till backlogg
//börjat med nätverk

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <time.h>
#include "sprinter.h"
#include "hunter.h"
#include "obstacle.h"
#include "game_states.h"
#include "network.h"

#undef main

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define HORIZONTAL_MARGIN 20 // Left & right boundary collision
#define NUM_OBSTACLES 23

typedef struct {
    int x;
    int y;
} sPosition;

typedef struct {
    int x;
    int y;
} hPosition; // Hunter spawn position

typedef struct {
    int x, y;
    bool active;
} Player;

typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;

bool init(SDL_Renderer **gRenderer);
void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSprinterSpriteClips[], SDL_Texture **mHunter, SDL_Rect gHunterSpriteClips[], SDL_Texture **mBackground, SDL_Texture **mMenu, SDL_Texture **mArrow);
void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mBackground);
void showTutorial(SDL_Renderer *gRenderer);
bool checkCollision(SDL_Rect a, SDL_Rect b);
void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, PlayerRole role, Obstacle obstacles[], int numObstacles);
void updateFrame(int *frame, PlayerRole role, int frame1, int frame2);
void drawDebugInfo(SDL_Renderer *gRenderer, Obstacle obstacles[], int numObstacles);
void updateGameState(GameState new_state);
void print_player_positions();

GameState current_state;
const int arrowYPositions[] = {100, 198, 288}; // Y-positions for our menu-options
Player players[MAX_CLIENTS];

UDPsocket sd;       // Socket descriptor
IPaddress srvadd;   // IP address for server
bool isServer = false;  // Mode switch

int main(int argc, char* argv[])
{
    char* host = "localhost"; // Standardhost
    Uint16 port = 2000;       // Standardport

    if (argc > 1) {
        if (strcmp(argv[1], "server") == 0) {
            isServer = true;
            port = 2000;              // Default port for server
        } else {
            host = argv[1];           // Use 1st argument as host
            if (argc > 2) {
                port = atoi(argv[2]); // If port is specified, use that as port
            } else {
                port = 12345;         // Default port for client if not specified
            }
        }
    }

    if (network_init(host, port, isServer) < 0) {
        printf("Network could not initialize!\n");
        SDL_Quit();
        return -1;
    }

    sPosition startPos[] = {
    {100, 64},   //1st pos
    {100, 550},  //2nd pos
    {1100, 64},  //3rd pos
    {1100, 550}   //4th pos
    };
    
    int numPositions = sizeof(startPos) / sizeof(startPos[0]);
    srand(time(NULL));
    int index = rand() % numPositions;

    SDL_Event e;
    SDL_Renderer *gRenderer = NULL;
    bool quit = false;
    
    // Sprinter
    SDL_Texture *mSprinter = NULL;
    SDL_Rect gSpriteClips[8];
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    SDL_Rect position;
    position.x = startPos[index].x;
    position.y = startPos[index].y;
    position.h = 32;
    position.w = 32;
    int frame = 6;

    //Background
    SDL_Texture *mBackground = NULL;

    // Hunter
    SDL_Texture *mHunter = NULL;
    SDL_Rect gHunterSpriteClips[8];  // Assuming 8 frames like Sprinter
    SDL_RendererFlip flipHunter = SDL_FLIP_NONE;
    SDL_Rect hunterPosition;
    hunterPosition.x = 600;
    hunterPosition.y = 300;
    hunterPosition.h = 32;
    hunterPosition.w = 32;
    int hunterFrame = 0;

    // Obstacles
    Obstacle obstacles[NUM_OBSTACLES];
    initObstacles(obstacles, NUM_OBSTACLES);

    //Menu
    SDL_Texture *mMenu = NULL;

    //Arrow in menu
    SDL_Texture *mArrow = NULL;

    if (init(&gRenderer)) {
        printf("Application starting...\n");
    }
    
    loadMedia(gRenderer, &mSprinter, gSpriteClips, &mHunter, gHunterSpriteClips, &mBackground, &mMenu, &mArrow);

    int arrowYPosIndex = 0; // Index for the arrows position in menu
    SDL_Rect arrowPos = {400, arrowYPositions[arrowYPosIndex], 40, 40};

    initializeGameState();
    setGameState(STATE_MENU);
    
    // Initialisera SDL för video och ljud
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialisera SDL_mixer, ställ in ljudfrekvens, ljudformat, antal ljudkanaler och chuckstorlek
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    Mix_Music *backgroundMusic = Mix_LoadMUS("resources/menusong3.mp3");
    if (!backgroundMusic) {
        fprintf(stderr, "Failed to load background music: %s\n", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    // Menu-loop
    bool showMenu = true;
    // Starta musiken om inte redan spelar
    if (!Mix_PlayingMusic()) {
        Mix_PlayMusic(backgroundMusic, -1);
    }
    while (showMenu && !quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        arrowYPosIndex--;
                        if (arrowYPosIndex < 0) {
                            arrowYPosIndex = STATE_TOTAL - 1; //Loops back to last alternative
                        }
                        break;
                    case SDLK_DOWN:
                        arrowYPosIndex++;
                        if (arrowYPosIndex >= STATE_TOTAL) {
                            arrowYPosIndex = 0; //Loops back to first alternative
                        }
                        break;
                    case SDLK_RETURN:
                        // Logic to handle choice based on arrowYPosIndex after Return-press
                        switch (arrowYPosIndex) {
                            case 0:
                                setGameState(STATE_PLAYING);
                                showMenu = false; // Closing menu and starting game
                                Mix_HaltMusic();
                                break;
                            case 1:
                                setGameState(STATE_TUTORIAL);
                                showTutorial(gRenderer);
                                showMenu = true;
                                break;
                            case 2:
                                setGameState(STATE_EXIT);
                                quit = true;
                                showMenu = false;
                                printf("Ending game.\n");
                                break;
                    }
                        break;
                    // ... other cases here ...
                }
            }
            arrowPos.y = arrowYPositions[arrowYPosIndex]; // Updating the arrows position based on users choice
        }
        
        // Handle network communication based on role
        if (isServer) {
            network_handle_server();
        } else {
            network_handle_client();
        }
        print_player_positions();
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        SDL_RenderCopy(gRenderer, mMenu, NULL, NULL);
        SDL_RenderCopy(gRenderer, mArrow, NULL, &arrowPos);
        SDL_RenderPresent(gRenderer);
    }

    PlayerRole playerRole = (rand() % 2 == 0) ? ROLE_SPRINTER : ROLE_HUNTER;

    // Game-loop
    while (!quit) {
    // Game event handling
    network_check_activity(0);
    if (isServer) {
    network_handle_server();  // Handle incoming connections and messages
    } else {
        network_handle_client();  // Handle client-logic
    }
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_u:
                    toggleObstacleDebugMode();
                    break;
                case SDLK_w:
                case SDLK_UP:
                    moveCharacter(playerRole == ROLE_SPRINTER ? &position : &hunterPosition, 0, -8, playerRole, obstacles, NUM_OBSTACLES);
                    updateFrame(&frame, playerRole, 4, 5);
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    moveCharacter(playerRole == ROLE_SPRINTER ? &position : &hunterPosition, 0, 8, playerRole, obstacles, NUM_OBSTACLES);
                    updateFrame(&frame, playerRole, 0, 1);
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    moveCharacter(playerRole == ROLE_SPRINTER ? &position : &hunterPosition, -8, 0, playerRole, obstacles, NUM_OBSTACLES);
                    if (playerRole == ROLE_SPRINTER) {
                        flip = SDL_FLIP_HORIZONTAL;
                    } else if (playerRole == ROLE_HUNTER) {
                        flipHunter = SDL_FLIP_HORIZONTAL;
                    }
                    updateFrame(&frame, playerRole, 2, 3);
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    moveCharacter(playerRole == ROLE_SPRINTER ? &position : &hunterPosition, 8, 0, playerRole, obstacles, NUM_OBSTACLES);
                    if (playerRole == ROLE_SPRINTER) {
                        flip = SDL_FLIP_NONE;
                    } else if (playerRole == ROLE_HUNTER) {
                        flipHunter = SDL_FLIP_NONE;
                    }
                    updateFrame(&frame, playerRole, 2, 3);
                    break;
                default:
                    break;
            }
        }
    }

        // Rendering
        SDL_RenderClear(gRenderer);
        renderBackground(gRenderer, mBackground);
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        drawObstacles(gRenderer, obstacles, NUM_OBSTACLES);
        if (playerRole == ROLE_SPRINTER) {
            SDL_RenderCopyEx(gRenderer, mSprinter, &gSpriteClips[frame], &position, 0, NULL, flip);
        } else if (playerRole == ROLE_HUNTER) {
            SDL_RenderCopyEx(gRenderer, mHunter, &gHunterSpriteClips[frame], &hunterPosition, 0, NULL, flipHunter);
        }
        
        SDL_RenderPresent(gRenderer);
    }
    network_cleanup();
    Mix_FreeMusic(backgroundMusic);
    Mix_CloseAudio();
    SDLNet_Quit();      //could be deleted since it exists in the function one line above?
    SDL_Quit();
    return 0;
}

void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mBackground) {
    SDL_RenderCopy(gRenderer, mBackground, NULL, NULL);
}

void showTutorial(SDL_Renderer *gRenderer) {
    // Load the tutorial image
    SDL_Surface* tutorialSurface = IMG_Load("resources/TUTORIAL.png");
    if (!tutorialSurface) {
        printf("Unable to load tutorial image: %s\n", IMG_GetError());
        return;
    }
    SDL_Texture* tutorialTexture = SDL_CreateTextureFromSurface(gRenderer, tutorialSurface);
    SDL_FreeSurface(tutorialSurface);
    SDL_Rect tutorialRect = {0, 0, 1280, 720};
    SDL_Event e;
    bool exitTutorial = false;
    // Tutorial event loop
    while (!exitTutorial) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                exitTutorial = true;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                exitTutorial = true; // Exit the tutorial on ESCAPE key
            }
        }
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        SDL_RenderCopy(gRenderer, tutorialTexture, NULL, &tutorialRect);
        SDL_RenderPresent(gRenderer);
    }
    SDL_DestroyTexture(tutorialTexture);
}

void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSprinterSpriteClips[], SDL_Texture **mHunter, SDL_Rect gHunterSpriteClips[], SDL_Texture **mBackground, SDL_Texture **mMenu, SDL_Texture **mArrow)
{    
    
    SDL_Surface* gSprinterSurface = IMG_Load("resources/SPRINTER.PNG");
    *mSprinter = SDL_CreateTextureFromSurface(gRenderer, gSprinterSurface);
    SDL_Surface* gHunterSurface = IMG_Load("resources/HUNTER.PNG");
    *mHunter = SDL_CreateTextureFromSurface(gRenderer, gHunterSurface);
  
    gSprinterSpriteClips[ 0 ].x = 0;
    gSprinterSpriteClips[ 0 ].y = 0;
    gSprinterSpriteClips[ 0 ].w = 16;
    gSprinterSpriteClips[ 0 ].h = 16;

    gSprinterSpriteClips[ 1 ].x = 16;
    gSprinterSpriteClips[ 1 ].y = 0;
    gSprinterSpriteClips[ 1 ].w = 16;
    gSprinterSpriteClips[ 1 ].h = 16;

    gSprinterSpriteClips[ 2 ].x = 32;
    gSprinterSpriteClips[ 2 ].y = 0;
    gSprinterSpriteClips[ 2 ].w = 16;
    gSprinterSpriteClips[ 2 ].h = 16;

    gSprinterSpriteClips[ 3 ].x = 48;
    gSprinterSpriteClips[ 3 ].y = 0;
    gSprinterSpriteClips[ 3 ].w = 16;
    gSprinterSpriteClips[ 3 ].h = 16;

    gSprinterSpriteClips[ 4 ].x = 64;
    gSprinterSpriteClips[ 4 ].y = 0;
    gSprinterSpriteClips[ 4 ].w = 16;
    gSprinterSpriteClips[ 4 ].h = 16;

    gSprinterSpriteClips[ 5 ].x = 80;
    gSprinterSpriteClips[ 5 ].y = 0;
    gSprinterSpriteClips[ 5 ].w = 16;
    gSprinterSpriteClips[ 5 ].h = 16;

    gSprinterSpriteClips[ 6 ].x = 96;
    gSprinterSpriteClips[ 6 ].y = 0;
    gSprinterSpriteClips[ 6 ].w = 16;
    gSprinterSpriteClips[ 6 ].h = 16;

    gSprinterSpriteClips[ 7 ].x = 112;
    gSprinterSpriteClips[ 7 ].y = 0;
    gSprinterSpriteClips[ 7 ].w = 16;
    gSprinterSpriteClips[ 7 ].h = 16;

    gHunterSpriteClips[ 0 ].x = 0;
    gHunterSpriteClips[ 0 ].y = 0;
    gHunterSpriteClips[ 0 ].w = 16;
    gHunterSpriteClips[ 0 ].h = 16;
    
    gHunterSpriteClips[ 1 ].x = 16;
    gHunterSpriteClips[ 1 ].y = 0;
    gHunterSpriteClips[ 1 ].w = 16;
    gHunterSpriteClips[ 1 ].h = 16;
    
    gHunterSpriteClips[ 2 ].x = 32;
    gHunterSpriteClips[ 2 ].y = 0;
    gHunterSpriteClips[ 2 ].w = 16;
    gHunterSpriteClips[ 2 ].h = 16;
    
    gHunterSpriteClips[ 3 ].x = 48;
    gHunterSpriteClips[ 3 ].y = 0;
    gHunterSpriteClips[ 3 ].w = 16;
    gHunterSpriteClips[ 3 ].h = 16;
    
    gHunterSpriteClips[ 4 ].x = 64;
    gHunterSpriteClips[ 4 ].y = 0;
    gHunterSpriteClips[ 4 ].w = 16;
    gHunterSpriteClips[ 4 ].h = 16;
    
    gHunterSpriteClips[ 5 ].x = 80;
    gHunterSpriteClips[ 5 ].y = 0;
    gHunterSpriteClips[ 5 ].w = 16;
    gHunterSpriteClips[ 5 ].h = 16;
    
    gHunterSpriteClips[ 6 ].x = 96;
    gHunterSpriteClips[ 6 ].y = 0;
    gHunterSpriteClips[ 6 ].w = 16;
    gHunterSpriteClips[ 6 ].h = 16;
    
    gHunterSpriteClips[ 7 ].x = 112;
    gHunterSpriteClips[ 7 ].y = 0;
    gHunterSpriteClips[ 7 ].w = 16;
    gHunterSpriteClips[ 7 ].h = 16;

    
    SDL_Surface* gBackgroundSurface = IMG_Load("resources/MAP.png");
    if (gBackgroundSurface == NULL) {
        printf("Unable to load background image: %s\n", IMG_GetError());
        // Handle the error, perhaps exit the function
    } else {
        *mBackground = SDL_CreateTextureFromSurface(gRenderer, gBackgroundSurface);
        SDL_FreeSurface(gBackgroundSurface);
        if (*mBackground == NULL) {
            printf("Unable to create background texture: %s\n", SDL_GetError());
        }
    }


    // Loading picture-file for arrow in menu
    SDL_Surface* gArrowSurface = IMG_Load("resources/ARROW.png");
    if (gArrowSurface == NULL) {
        printf("Unable to load arrow image: %s\n", IMG_GetError());
    } else {
        *mArrow = SDL_CreateTextureFromSurface(gRenderer, gArrowSurface);
        if (*mArrow == NULL) {
            printf("Unable to create texture from arrow surface: %s\n", SDL_GetError());
        }
        SDL_FreeSurface(gArrowSurface);
    }

    // Loading picture-file for menu
    SDL_Surface* gMenuSurface = IMG_Load("resources/MENU.png");
    if (gMenuSurface != NULL) {
        *mMenu = SDL_CreateTextureFromSurface(gRenderer, gMenuSurface);
        if (*mMenu == NULL) {
            printf("Unable to create texture from menu surface: %s\n", SDL_GetError());
        }
        SDL_FreeSurface(gMenuSurface);
    } else {
        printf("Unable to load menu image: %s\n", IMG_GetError());
    }
}

bool init(SDL_Renderer **gRenderer) {
    bool test = true;
    SDL_Window  *gWindow = NULL;
    SDL_Init(SDL_INIT_VIDEO);
    gWindow = SDL_CreateWindow("SDL Test", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
    if(gWindow == NULL) {
        printf("Fungerar ej\n");
        test = false;
    }
    *gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(*gRenderer == NULL) {
        printf("Fungerar ej\n");
        test = false;
    }
    return test;
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
    // Check if there's no overlap
    if (a.x + a.w <= b.x || b.x + b.w <= a.x ||
        a.y + a.h <= b.y || b.y + b.h <= a.y) {
        return false;
    }
    return true;
}

void moveCharacter(SDL_Rect *charPos, int deltaX, int deltaY, PlayerRole role, Obstacle obstacles[], int numObstacles) {
    SDL_Rect newPos = *charPos;
    newPos.x += deltaX;
    newPos.y += deltaY;

    for (int i = 0; i < numObstacles; i++) {
        if (checkCollision(newPos, obstacles[i].bounds)) {
            return;  // Collision detected, do not update position
        }
    }
    newPos.x = SDL_clamp(newPos.x, HORIZONTAL_MARGIN, WINDOW_WIDTH - newPos.w - HORIZONTAL_MARGIN);
    newPos.y = SDL_clamp(newPos.y, 0, WINDOW_HEIGHT - newPos.h);

    *charPos = newPos;
}

void updateFrame(int *frame, PlayerRole role, int frame1, int frame2) {
    // Switch frame for animation
    *frame = *frame == frame1 ? frame2 : frame1;
}

void drawDebugInfo(SDL_Renderer *gRenderer, Obstacle obstacles[], int numObstacles) {
    SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 128);  // Red color for collision boxes, semi-transparent
    for (int i = 0; i < numObstacles; i++) {
        SDL_RenderDrawRect(gRenderer, &obstacles[i].bounds);  // Draw rectangle around the collision area
    }
}

void updateGameState(GameState new_state) {
    current_state = new_state;
    // Additional logic to handle state change
}

void print_player_positions() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (players[i].active) {
            printf("Player %d: x = %d, y = %d\n", i, players[i].x, players[i].y);
        }
    }
}