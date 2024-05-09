#ifndef PERK_H
#define PERK_H

#include <SDL2/SDL.h>


typedef struct parks Parks;
typedef enum{
    PARKS_SERVER,
    PARKS_CLIENT
} ParksState;

Parks *creatPerk(SDL_Renderer *pRenderer, ParksState parksState, SDL_Rect *obstacleRects);
void setPosition(Parks *pParks, SDL_Rect *obstacleRects);
int checkPosition(SDL_Rect pParkRect, SDL_Rect *obstacleRects);
void drawPerk(Parks *pParks);
void destroyPerk(Parks *pParks);
void setPerkX(Parks *pParks, int x);
void setPerkY(Parks *pParks, int y);
SDL_Rect getPerkRECT(Parks *pParks);
int getPerkX(Parks *pParks);
int getPerkY(Parks *pParks);
int checkCollisionPerk(SDL_Rect a, SDL_Rect b);
#endif