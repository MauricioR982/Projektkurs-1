#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <SDL2/SDL.h>

typedef struct {
    SDL_Rect bounds; //x, y , w, h
} Obstacle;

void initObstacles(Obstacle obstacles[], int size);
void drawObstacles(SDL_Renderer* renderer, Obstacle obstacles[], int size);

#endif
