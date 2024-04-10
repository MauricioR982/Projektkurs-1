//
//  main.c
//  SDLtesta
//
//  Created by Jonas Willén on 2021-03-29.
//
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_image.h> 
//

#include <stdio.h>
//#include <SDL.h>
//#include <SDL_image.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 
#include <stdbool.h>
#include "world.h"
#include "Alien.h"
#include <time.h>
#include "sprinter.h"

#undef main

bool init(SDL_Renderer **gRenderer);
void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSpriteClips[], SDL_Texture **mAlien, SDL_Rect gAlien[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow);
void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTile, SDL_Rect gTiles[]);

typedef struct {
    int x;
    int y;
} sPosition;

typedef struct {
    int x;
    int y;
} hPosition; //hunter spawn position lägg till funktionalitet


typedef enum {
    MENU_START_GAME,
    MENU_MULTIPLAYER,
    MENU_TUTORIAL,
    MENU_EXIT,
    MENU_TOTAL //Antal menyalternativ
} MenuOption;

MenuOption currentOption = MENU_START_GAME;


int main(int argc, char* args[])
{
    SDL_Rect arrowPositions[MENU_TOTAL] = {
    {400, 220}, // Position för "START GAME"
    {400, 320}, // Position för "3-5 Players"
    {400, 420}, // Position för "Exit"
    {400, 520}  // Position för "TUTORIAL"
    };

    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;
    const int HORIZONTAL_MARGIN = 20; // vänster och höger kant kollision

    sPosition startPos[] = {
    {110, 64},   // Forsta position
    {110, 550},  // Andra position
    {1100, 64},  // Tredje position
    {1100, 550}   // Fjarde position
    };
    
    int numPositions = sizeof(startPos) / sizeof(startPos[0]);

    // At the beginning of your main function
    srand(time(NULL)); // Seed the random number generator
    int index = rand() % numPositions; // Randomly select an index

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
    
    // Alien 1 and 2
    SDL_Texture *mAlien = NULL;
    SDL_Rect gAlien[2];
    Alien a1;
    a1 = createAlien(41, 100);
    SDL_Rect a1possition = {getAlienPositionY(a1),getAlienPositionX(a1),15,15};
    
    Alien a2;
    a2 = createAlien(80, 100);
    SDL_Rect a2possition = {getAlienPositionY(a1),getAlienPositionX(a1),15,15};
   
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
    
    loadMedia(gRenderer, &mSprinter, gSpriteClips, &mAlien, gAlien, &mTiles, gTiles, &mMenu, &mArrow);
    
    //Menu-loop
    bool showMenu = true;   // A boolean flag to assure that the menu is shown
    while (showMenu && !quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        currentOption = (currentOption - 1 + MENU_TOTAL) % MENU_TOTAL;
                        break;
                    case SDLK_DOWN:
                        currentOption = (currentOption + 1) % MENU_TOTAL;
                        break;
                    case SDLK_RETURN:
                        switch (currentOption) {
                            case MENU_START_GAME:
                                // Starta spelet eller vad du nu vill göra här
                                showMenu = false;
                                break;
                            case MENU_MULTIPLAYER:
                                // Gå till multiplayer-spelet
                                break;
                            case MENU_TUTORIAL:
                                // Visa tutorial
                                break;
                            case MENU_EXIT:
                                quit = true; // Avsluta spelet
                                break;
                        }
                        break;
                }
        }
    }

    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF); // Vit bakgrundsfärg
    SDL_RenderClear(gRenderer); // Rensar renderaren
    
    if (mMenu != NULL) {
        SDL_RenderCopy(gRenderer, mMenu, NULL, NULL); // Ritar ut menybilden
    } else {
        printf("Menu texture is not available.\n");
    }

    SDL_Rect arrowPos = arrowPositions[currentOption]; // Hämtar den nuvarande pilpositionen
    if (mArrow != NULL) {
        SDL_RenderCopy(gRenderer, mArrow, NULL, &arrowPos); // Ritar ut pilen
    } else {
        printf("Arrow texture is not available.\n");
    }

    SDL_RenderPresent(gRenderer); // Presenterar renderaren
}



    // Game loop - 1. Game Event 2. Game Logic 3. Render Game
    while (!quit) {
    // Game event
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if( e.type == SDL_KEYDOWN )
        {
            //Select surfaces based on key press
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
        
        // Game logic use ATD 
        AlienTick(a1);
        a1possition.x = getAlienPositionY(a1);
        a1possition.y = getAlienPositionX(a1);
        int foo = collidesWithImpassableTile(position.x, position.y);
        
        // Game renderer
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        renderBackground(gRenderer, mTiles, gTiles);
        SDL_RenderCopyEx(gRenderer, mSprinter, &gSpriteClips[frame],&position , 0, NULL, flip);
        SDL_RenderCopyEx(gRenderer, mAlien, &gAlien[getAlienFrame(a1)],&a1possition ,270, NULL, SDL_FLIP_NONE);
        SDL_RenderCopyEx(gRenderer, mAlien, &gAlien[getAlienFrame(a2)],&a2possition ,270, NULL, SDL_FLIP_NONE);
        SDL_RenderPresent(gRenderer);
    }
    
    return 0;
}

void renderBackground(SDL_Renderer *gRenderer, SDL_Texture *mTiles, SDL_Rect gTiles[])
{
    SDL_RenderCopy(gRenderer, mTiles, NULL, NULL);
}

void loadMedia(SDL_Renderer *gRenderer, SDL_Texture **mSprinter, SDL_Rect gSpriteClips[], SDL_Texture **mAlien, SDL_Rect gAlien[], SDL_Texture **mTiles, SDL_Rect gTiles[], SDL_Texture **mMenu, SDL_Texture **mArrow){
    
    SDL_Surface* gSprinterSurface = IMG_Load("resources/SPACEMAN.PNG");
    *mSprinter = SDL_CreateTextureFromSurface(gRenderer, gSprinterSurface);
  
    gSpriteClips[ 0 ].x =   0;
    gSpriteClips[ 0 ].y =   0;
    gSpriteClips[ 0 ].w =  16;
    gSpriteClips[ 0 ].h = 16;
    
    gSpriteClips[ 1 ].x =  16;
    gSpriteClips[ 1 ].y =   0;
    gSpriteClips[ 1 ].w =  16;
    gSpriteClips[ 1 ].h = 16;
    
    gSpriteClips[ 2 ].x = 32;
    gSpriteClips[ 2 ].y =   0;
    gSpriteClips[ 2 ].w =  16;
    gSpriteClips[ 2 ].h = 16;
    
    gSpriteClips[ 3 ].x = 48;
    gSpriteClips[ 3 ].y =   0;
    gSpriteClips[ 3 ].w =  16;
    gSpriteClips[ 3 ].h = 16;
    
    gSpriteClips[ 4 ].x = 64;
    gSpriteClips[ 4 ].y =   0;
    gSpriteClips[ 4 ].w =  16;
    gSpriteClips[ 4 ].h = 16;
    
    gSpriteClips[ 5 ].x = 80;
    gSpriteClips[ 5 ].y =   0;
    gSpriteClips[ 5 ].w =  16;
    gSpriteClips[ 5 ].h = 16;
    
    gSpriteClips[ 6 ].x = 96;
    gSpriteClips[ 6 ].y =   0;
    gSpriteClips[ 6 ].w =  16;
    gSpriteClips[ 6 ].h = 16;
    
    gSpriteClips[ 7 ].x = 112;
    gSpriteClips[ 7 ].y =   0;
    gSpriteClips[ 7 ].w =  16;
    gSpriteClips[ 7 ].h = 16;
    
    SDL_Surface* gTilesSurface = IMG_Load("resources/TILES.PNG");
    *mTiles = SDL_CreateTextureFromSurface(gRenderer, gTilesSurface);
    for (int i = 0; i < 16; i++) {
        gTiles[i].x = i*getTileWidth();
        gTiles[i].y = 0;
        gTiles[i].w = getTileWidth();
        gTiles[i].h = getTileHeight();
    }
    
    //SDL_Surface* gSAlien = IMG_Load("resources/ALIEN.PNG");
    //*mAlien = SDL_CreateTextureFromSurface(gRenderer, gSAlien);
    gAlien[ 0 ].x = 0;
    gAlien[ 0 ].y = 0;
    gAlien[ 0 ].w = 15;
    gAlien[ 0 ].h = 15;
    
    gAlien[ 1 ].x = 15;
    gAlien[ 1 ].y = 0;
    gAlien[ 1 ].w = 15;
    gAlien[ 1 ].h = 15;
    

    SDL_Surface* gBackgroundSurface = IMG_Load("resources/Map.png");
    if(gBackgroundSurface == NULL) {
        printf("Kunde inte ladda bakgrundsbild: %s\n", IMG_GetError());
    } else {
        *mTiles = SDL_CreateTextureFromSurface(gRenderer, gBackgroundSurface);
        SDL_FreeSurface(gBackgroundSurface);
    }

    // Load arrow for menu
    SDL_Surface* gArrowSurface = IMG_Load("resources/arrow1.png");
    if (gArrowSurface == NULL) {
        printf("Unable to load arrow image: %s\n", IMG_GetError());
        // Eventuell felhantering här
    } else {
        *mArrow = SDL_CreateTextureFromSurface(gRenderer, gArrowSurface);
        if (*mArrow == NULL) {
            printf("Unable to create texture from arrow surface: %s\n", SDL_GetError());
            // Eventuell felhantering här
        }
        SDL_FreeSurface(gArrowSurface);
}

    // Load picture-file for menu
    SDL_Surface* gMenuSurface = IMG_Load("resources/MENU.png");
    if (gMenuSurface != NULL) {
        *mMenu = SDL_CreateTextureFromSurface(gRenderer, gMenuSurface);
        if (*mMenu == NULL) {
            printf("Unable to create texture from menu surface: %s\n", SDL_GetError());
            // Eventuell ytterligare felhantering här
        }
        SDL_FreeSurface(gMenuSurface);
    } else {
        printf("Unable to load menu image: %s\n", IMG_GetError());
    }


}


bool init(SDL_Renderer **gRenderer){
    bool test = true;
    SDL_Window  *gWindow = NULL;
    SDL_Init(SDL_INIT_VIDEO);
    gWindow = SDL_CreateWindow("SDL Test", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
    if(gWindow == NULL){
        printf("Fungerar ej\n");
        test = false;
    }
    *gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED| SDL_RENDERER_PRESENTVSYNC);
    if(*gRenderer == NULL){
        printf("Fungerar ej\n");
        test = false;
    }
    return test;
    
}