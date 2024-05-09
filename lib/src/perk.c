#include <SDL2/SDL.h>
#include <stdlib.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include "perk.h"

#define PARKS_WIDTH 15
#define PARKS_HEIGHT 15
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define NUM_OBSTACLES 23

struct parks
{
    SDL_Renderer *pRenderer;
    SDL_Rect pParksRect;
    SDL_Texture *pTexture;
};

Parks *creatPerk(SDL_Renderer *pRenderer, ParksState parksState, SDL_Rect *obstacleRects){
    Parks *pParks = malloc(sizeof(pParks));
    pParks->pRenderer = pRenderer;

    SDL_Surface *pSurface = IMG_Load("../lib/resources/park.png");
    if (!pSurface)
    {
        printf("Error: %s\n", SDL_GetError());
        free(pParks);
        return NULL;
    }
    pParks->pTexture = SDL_CreateTextureFromSurface(pParks->pRenderer, pSurface);
    SDL_FreeSurface(pSurface);
    if (!pParks->pTexture)
    {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }
    srand(time(NULL));
    if (parksState == PARKS_SERVER)
        setPosition(pParks, obstacleRects);
    else if (parksState == PARKS_CLIENT)
    {
        pParks->pParksRect.x = pParks->pParksRect.y = 0;
        pParks->pParksRect.w = PARKS_WIDTH;
        pParks->pParksRect.h = PARKS_HEIGHT;
    }

    return pParks;
}

void setPosition(Parks *pParks, SDL_Rect *obstacleRects){
    float randX, randY;
    randX = rand() % WINDOW_WIDTH +15;
    randY = rand() % WINDOW_HEIGHT+15;
    SDL_Rect rect = {randX, randY, PARKS_WIDTH, PARKS_HEIGHT};
    if (checkPosition(rect, obstacleRects))
        pParks->pParksRect = rect;
    else
        setPosition(pParks, obstacleRects);
}

int checkPosition(SDL_Rect parksRect, SDL_Rect *obstacleRects){
    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        
        if (checkCollisionPerk(parksRect, obstacleRects[i]))
        {
            printf("krock");
            return 0;
        }
    }
    return 1;
}

void destroyPerk(Parks *pParks){
    free(pParks);
}

int getPerkX(Parks *pParks){
    return pParks->pParksRect.x;
}
int getPerkY(Parks *pParks){
    return pParks->pParksRect.y;
}
void setPerkX(Parks *pParks, int x){
    pParks->pParksRect.x = x;
}
void setPerkY(Parks *pParks, int y){
    pParks->pParksRect.y = y;
}
SDL_Rect getPerkRECT(Parks *pParks){
    return pParks->pParksRect;
}

void drawPerk(Parks *pParks){
    SDL_RenderCopyEx(pParks->pRenderer, pParks->pTexture, NULL, &(pParks->pParksRect), 0, NULL, SDL_FLIP_NONE);
}

int checkCollisionPerk(SDL_Rect a, SDL_Rect b) {
    // Check if there's no overlap
    if (a.x + a.w <= b.x || b.x + b.w <= a.x ||
        a.y + a.h <= b.y || b.y + b.h <= a.y) {
        return 0;
    }
    return 1;
}