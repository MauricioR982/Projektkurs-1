//
//  'main.c'
//  Developed by Grupp 10 - Datateknik, project-start at 2024-03.
//  @"f.e." = for example
//

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 
#include <stdbool.h>
#include "world.h"
#include <time.h>
#include "sprinter.h"
#include "hunter.h"

#undef main

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define HORIZONTAL_MARGIN 20 // left & right boundary collision

bool init(SDL_Renderer **gRenderer);
void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSprinterSpriteClips[], SDL_Texture **mHunter, SDL_Rect gHunterSpriteClips[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow);
void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTile, SDL_Rect gTiles[]);
void showTutorial(SDL_Renderer *gRenderer);
void movePlayer(SDL_Rect *playerPosition, SDL_Rect direction, SDL_Rect *obstacles, int numObstacles);
bool checkCollision(SDL_Rect a, SDL_Rect b);

typedef struct {
    int x;
    int y;
} sPosition;

typedef struct {
    int x;
    int y;
} hPosition; //Hunter spawn position

typedef enum {
    MENU_START_GAME,
    MENU_MULTIPLAYER,
    MENU_TUTORIAL,
    MENU_EXIT,
    MENU_TOTAL
} MenuOption;

typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;

MenuOption currentOption = MENU_START_GAME;
const int arrowYPositions[] = {100, 165, 228, 287}; // Y-positions for our menu-options

    SDL_Rect trees[] = {
        {200, 150, 50, 50},  // x position, y position, width, height
        {400, 300, 50, 50}
    };

    SDL_Rect rocks[] = {
        {600, 400, 50, 50},
        {800, 200, 50, 50}
    };


int main(int argc, char* args[])
{
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
    
    // Background
    SDL_Texture *mTiles = NULL;
    SDL_Rect gTiles[16];

    //Menu
    SDL_Texture *mMenu = NULL;

    //Arrow in menu
    SDL_Texture *mArrow = NULL;

    if (init(&gRenderer)) {
        printf("worked\n");
    }
    
    loadMedia(gRenderer, &mSprinter, gSpriteClips, &mHunter, gHunterSpriteClips, &mTiles, gTiles, &mMenu, &mArrow);

    int arrowYPosIndex = 0; // Index for the arrows position in menu
    SDL_Rect arrowPos = {400, arrowYPositions[arrowYPosIndex], 40, 40}; 

    // Menu-loop
    bool showMenu = true;
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
                            arrowYPosIndex = MENU_TOTAL - 1; //Loops back to last alternative
                        }
                        break;
                    case SDLK_DOWN:
                        arrowYPosIndex++;
                        if (arrowYPosIndex >= MENU_TOTAL) {
                            arrowYPosIndex = 0; //Loops back to first alternative
                        }
                        break;
                    case SDLK_RETURN:
                        // Logic to handle choice based on arrowYPosIndex after Return-press
                        switch (arrowYPosIndex) {
                        case MENU_START_GAME:
                            showMenu = false; // Closing menu and starting game
                            break;
                        case MENU_TUTORIAL:
                            showTutorial(gRenderer);
                            //showMenu = true;
                            break;
                        case MENU_EXIT:
                            quit = true;
                            showMenu = false;
                            break;
                    }
                        break;
                    // ... other cases here ...
                }
            }
            arrowPos.y = arrowYPositions[arrowYPosIndex]; // Updating the arrows position based on users choice
        }
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        SDL_RenderCopy(gRenderer, mMenu, NULL, NULL);
        SDL_RenderCopy(gRenderer, mArrow, NULL, &arrowPos);
        SDL_RenderPresent(gRenderer);
    }

    PlayerRole playerRole = (rand() % 2 == 0) ? ROLE_SPRINTER : ROLE_HUNTER;

    // Game-loop
    while (!quit) {
    // Game event
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        } else if (e.type == SDL_KEYDOWN) {
            SDL_Rect moveDirection = {0, 0, 0, 0}; // Initialize move direction
            bool isSprinter = (playerRole == ROLE_SPRINTER);
            SDL_Rect* currentPosition = isSprinter ? &position : &hunterPosition;
            SDL_RendererFlip* currentFlip = isSprinter ? &flip : &flipHunter;
            int* currentFrame = isSprinter ? &frame : &hunterFrame;
            SDL_Rect* obstacles = isSprinter ? trees : rocks;
            int numObstacles = isSprinter ? sizeof(trees) / sizeof(trees[0]) : sizeof(rocks) / sizeof(rocks[0]);

            switch (e.key.keysym.sym) {
                case SDLK_w:
                case SDLK_UP:
                    moveDirection.y = -8;
                    movePlayer(currentPosition, moveDirection, obstacles, numObstacles);
                    *currentFlip = SDL_FLIP_NONE;
                    *currentFrame = (*currentFrame == 4) ? 5 : 4;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    moveDirection.y = 8;
                    movePlayer(currentPosition, moveDirection, obstacles, numObstacles);
                    *currentFlip = SDL_FLIP_NONE;
                    *currentFrame = (*currentFrame == 0) ? 1 : 0;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    moveDirection.x = -8;
                    movePlayer(currentPosition, moveDirection, obstacles, numObstacles);
                    *currentFlip = SDL_FLIP_HORIZONTAL;
                    *currentFrame = (*currentFrame == 2) ? 3 : 2;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    moveDirection.x = 8;
                    movePlayer(currentPosition, moveDirection, obstacles, numObstacles);
                    *currentFlip = SDL_FLIP_NONE;
                    *currentFrame = (*currentFrame == 2) ? 3 : 2;
                    break;
            }

            // Check for screen boundary limits
            currentPosition->x = SDL_clamp(currentPosition->x, HORIZONTAL_MARGIN, WINDOW_WIDTH - currentPosition->w - HORIZONTAL_MARGIN);
            currentPosition->y = SDL_clamp(currentPosition->y, 0, WINDOW_HEIGHT - currentPosition->h);
        }
    }

    // Render updates
    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(gRenderer);
    renderBackground(gRenderer, mTiles, gTiles);
    if (playerRole == ROLE_SPRINTER) {
        SDL_RenderCopyEx(gRenderer, mSprinter, &gSpriteClips[frame], &position, 0, NULL, flip);
    } else if (playerRole == ROLE_HUNTER) {
        SDL_RenderCopyEx(gRenderer, mHunter, &gHunterSpriteClips[hunterFrame], &hunterPosition, 0, NULL, flipHunter);
    }

    for (int i = 0; i < sizeof(trees) / sizeof(trees[0]); i++) {
        SDL_RenderFillRect(gRenderer, &trees[i]);
    }

    for (int i = 0; i < sizeof(rocks) / sizeof(rocks[0]); i++) {
        SDL_RenderFillRect(gRenderer, &rocks[i]);
    }

    SDL_RenderPresent(gRenderer);
    }
    return 0;
}

void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTiles, SDL_Rect gTiles[])
{
    SDL_RenderCopy(gRenderer, mTiles, NULL, NULL);
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

void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSprinterSpriteClips[], SDL_Texture **mHunter, SDL_Rect gHunterSpriteClips[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow){    
    
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
    
    SDL_Surface* gTilesSurface = IMG_Load("resources/TILES.PNG");
    *mTiles = SDL_CreateTextureFromSurface(gRenderer, gTilesSurface);
    for (int i = 0; i < 16; i++) {
        gTiles[i].x = i*getTileWidth();
        gTiles[i].y = 0;
        gTiles[i].w = getTileWidth();
        gTiles[i].h = getTileHeight();
    }
    
    //Loading picture-file for Map
    SDL_Surface* gBackgroundSurface = IMG_Load("resources/MAP.png");
    if(gBackgroundSurface == NULL) {
        printf("Kunde inte ladda bakgrundsbild: %s\n", IMG_GetError());
    } else {
        *mTiles = SDL_CreateTextureFromSurface(gRenderer, gBackgroundSurface);
        SDL_FreeSurface(gBackgroundSurface);
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
    SDL_Surface* gMenuSurface = IMG_Load("resources/newMenu.png");
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

void movePlayer(SDL_Rect *playerPosition, SDL_Rect direction, SDL_Rect *obstacles, int numObstacles) {
    SDL_Rect newPosition = *playerPosition;
    newPosition.x += direction.x;
    newPosition.y += direction.y;

    for (int i = 0; i < numObstacles; i++) {
        if (checkCollision(newPosition, obstacles[i])) {
            return;  // Collision detected, do not update player position
        }
    }

    // No collision, update player position
    playerPosition->x = newPosition.x;
    playerPosition->y = newPosition.y;
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
    return SDL_HasIntersection(&a, &b);
}
