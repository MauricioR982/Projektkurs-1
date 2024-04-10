//
//  'main.c'
//  Developed by Grupp 10 - Datateknik, on 2024-04-09.
//

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 
#include <stdbool.h>
#include "world.h"
#include <time.h>
#include "sprinter.h"

#undef main

bool init(SDL_Renderer **gRenderer);
void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSpriteClips[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow);
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
    MENU_TUTORIAL,
    MENU_EXIT,
    MENU_TOTAL //Nr of menuoptions
} MenuOption;

MenuOption currentOption = MENU_START_GAME;

int main(int argc, char* args[])
{
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;
    const int HORIZONTAL_MARGIN = 20; // left & right boundary collision

    sPosition startPos[] = {
    {110, 64},   //1st pos
    {110, 550},  //2nd pos
    {1100, 64},  //3rd pos
    {1100, 550}   //4th pos
    };
    
    int numPositions = sizeof(startPos) / sizeof(startPos[0]);
    srand(time(NULL));
    int index = rand() % numPositions;

    SDL_Event e;
    SDL_Renderer *gRenderer = NULL;
    bool quit = false;
    
    // Spaceman
    SDL_Texture *mSprinter = NULL;
    SDL_Rect gSpriteClips[8];
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    SDL_Rect position;
    position.x = startPos[index].x;
    position.y = startPos[index].y;
    position.h = 32;
    position.w = 32;
    int frame = 6;
    
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
    
    loadMedia(gRenderer, &mSprinter, gSpriteClips, &mTiles, gTiles, &mMenu, &mArrow);

    //Menu-loop
    bool showMenu = true; // A flag to make shure menu shall be shown
    while (showMenu && !quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                // User-input for the menu is handled here (f.e. start game or quit)
                // if user presses Enter, close the menu:
                if (e.key.keysym.sym == SDLK_RETURN) {
                    showMenu = false; // Closing the menu-loop & on to the game
                }
                // Add code here for the other menuoptions (Tutorial, Exit ...)
            }
        }
        
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF); // White background-color
        SDL_RenderClear(gRenderer);
        SDL_RenderCopy(gRenderer, mMenu, NULL, NULL);
        SDL_Rect testArrowPos = {400, 100, 40, 40}; // Little rectangle for the arrow
        SDL_RenderCopy(gRenderer, mArrow, NULL, &testArrowPos);
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
        SDL_RenderCopyEx(gRenderer, mSprinter, &gSpriteClips[frame],&position , 0, NULL, flip);
        SDL_RenderPresent(gRenderer);
    }
    return 0;
}

void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTiles, SDL_Rect gTiles[])
{
    SDL_RenderCopy(gRenderer, mTiles, NULL, NULL);
}

void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSpriteClips[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow){
    
    SDL_Surface* gSprinterSurface = IMG_Load("resources/SPACEMAN.PNG");
    *mSprinter = SDL_CreateTextureFromSurface(gRenderer, gSprinterSurface);
  
    gSpriteClips[ 0 ].x = 0;
    gSpriteClips[ 0 ].y = 0;
    gSpriteClips[ 0 ].w = 16;
    gSpriteClips[ 0 ].h = 16;
    
    gSpriteClips[ 1 ].x = 16;
    gSpriteClips[ 1 ].y = 0;
    gSpriteClips[ 1 ].w = 16;
    gSpriteClips[ 1 ].h = 16;
    
    gSpriteClips[ 2 ].x = 32;
    gSpriteClips[ 2 ].y = 0;
    gSpriteClips[ 2 ].w = 16;
    gSpriteClips[ 2 ].h = 16;
    
    gSpriteClips[ 3 ].x = 48;
    gSpriteClips[ 3 ].y = 0;
    gSpriteClips[ 3 ].w = 16;
    gSpriteClips[ 3 ].h = 16;
    
    gSpriteClips[ 4 ].x = 64;
    gSpriteClips[ 4 ].y = 0;
    gSpriteClips[ 4 ].w = 16;
    gSpriteClips[ 4 ].h = 16;
    
    gSpriteClips[ 5 ].x = 80;
    gSpriteClips[ 5 ].y = 0;
    gSpriteClips[ 5 ].w = 16;
    gSpriteClips[ 5 ].h = 16;
    
    gSpriteClips[ 6 ].x = 96;
    gSpriteClips[ 6 ].y = 0;
    gSpriteClips[ 6 ].w = 16;
    gSpriteClips[ 6 ].h = 16;
    
    gSpriteClips[ 7 ].x = 112;
    gSpriteClips[ 7 ].y = 0;
    gSpriteClips[ 7 ].w = 16;
    gSpriteClips[ 7 ].h = 16;
    
    
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
    SDL_Surface* gMenuSurface = IMG_Load("resources/MENU.png");
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