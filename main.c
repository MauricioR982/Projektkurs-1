//
//  'main.c'
//  Developed by Grupp 10 - Datateknik, on 2024-04-09.
//  !note!: "f.e." = for example
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

bool init(SDL_Renderer **gRenderer);
void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSprinterSpriteClips[], SDL_Texture **mHunter, SDL_Rect gHunterSpriteClips[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow);
void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTile, SDL_Rect gTiles[]);

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
    MENU_EXIT,
    MENU_TUTORIAL,
    MENU_TOTAL
} MenuOption;

MenuOption currentOption = MENU_START_GAME;
const int arrowYPositions[] = {100, 165, 220, 290}; // Y-positions for our menu-options

int main(int argc, char* args[])
{
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;
    const int HORIZONTAL_MARGIN = 20; // left & right boundary collision

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
                        case MENU_EXIT:
                            quit = true;
                            showMenu = false;
                            break;
                    case MENU_TUTORIAL:
                            showTutorial(gRenderer); // This will be your new tutorial function
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

    // Game-loop
    while (!quit) {
    // Game event
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if( e.type == SDL_KEYDOWN )
        {
            //Select surfaces based on keypress
            switch( e.key.keysym.sym )
            {
                case SDLK_w:
                case SDLK_UP:
                    position.y -= 8;
                    flip = SDL_FLIP_NONE;
                    if(frame == 4)
                        frame = 5;
                    else
                        frame = 4;
                    break;
                case SDLK_DOWN:    
                case SDLK_s:
                    position.y += 8;
                    flip = SDL_FLIP_NONE;
                    if(frame == 0)
                        frame = 1;
                    else
                        frame = 0;
                    break;
                case SDLK_LEFT:    
                case SDLK_a:
                    position.x -= 8;
                    flip = SDL_FLIP_HORIZONTAL;
                    if(frame == 2)
                        frame = 3;
                    else
                        frame = 2;
                    break;
                case SDLK_RIGHT:    
                case SDLK_d:
                    position.x += 8; 
                    flip = SDL_FLIP_NONE;
                    if(frame == 2)
                        frame = 3;
                    else
                        frame = 2;
                    break;
                default:
                    break;
            }
            if (position.x < HORIZONTAL_MARGIN) {
                position.x = HORIZONTAL_MARGIN;
            } else if (position.x + position.w > WINDOW_WIDTH - HORIZONTAL_MARGIN) {
                position.x = WINDOW_WIDTH - position.w - HORIZONTAL_MARGIN;
            }

            if (position.y < 0) {
                position.y = 0;
            } else if (position.y + position.h > WINDOW_HEIGHT) {
                position.y = WINDOW_HEIGHT - position.h;
            }
        }
    }
        // Game renderer
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        renderBackground(gRenderer, mTiles, gTiles);
        SDL_RenderCopyEx(gRenderer, mSprinter, &gSpriteClips[frame], &position, 0, NULL, flip);
        SDL_RenderCopyEx(gRenderer, mHunter, &gHunterSpriteClips[hunterFrame], &hunterPosition, 0, NULL, flip);
        SDL_RenderPresent(gRenderer);
    }
    return 0;
}

void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTiles, SDL_Rect gTiles[])
{
    SDL_RenderCopy(gRenderer, mTiles, NULL, NULL);
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
        // Ev. error handling here
    } else {
        *mArrow = SDL_CreateTextureFromSurface(gRenderer, gArrowSurface);
        if (*mArrow == NULL) {
            printf("Unable to create texture from arrow surface: %s\n", SDL_GetError());
            // Ev. error handling here
        }
        SDL_FreeSurface(gArrowSurface);
    }

    // Loading picture-file for menu
    SDL_Surface* gMenuSurface = IMG_Load("resources/NEWMENU.png");
    if (gMenuSurface != NULL) {
        *mMenu = SDL_CreateTextureFromSurface(gRenderer, gMenuSurface);
        if (*mMenu == NULL) {
            printf("Unable to create texture from menu surface: %s\n", SDL_GetError());
            // Ev. error handling here
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

void showTutorial(SDL_Renderer *gRenderer) {
    // Load tutorial image or text
    SDL_Surface* tutorialSurface = IMG_Load("resources/TUTORIAL.png");
    if (!tutorialSurface) {
        printf("Unable to load tutorial image: %s\n", IMG_GetError());
        return; // Add error handling as appropriate
    }
    SDL_Texture* tutorialTexture = SDL_CreateTextureFromSurface(gRenderer, tutorialSurface);
    SDL_FreeSurface(tutorialSurface); // We can free the surface as we don't need it after texture creation

    SDL_Rect tutorialRect = {0, 0, 1280, 720}; // Assuming the tutorial is full screen

    SDL_Event e;
    bool exitTutorial = false;

    // Tutorial event loop
    while (!exitTutorial) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                exitTutorial = true;
            } else if (e.type == SDL_KEYDOWN) {
                // Assuming pressing ESCAPE will exit the tutorial
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    exitTutorial = true;
                }
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);

        // Render tutorial texture
        SDL_RenderCopy(gRenderer, tutorialTexture, NULL, &tutorialRect);

        // Update the screen
        SDL_RenderPresent(gRenderer);
    }

    // Clean up
    SDL_DestroyTexture(tutorialTexture);
}