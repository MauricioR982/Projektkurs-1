#include "obstacle.h"
#include <stdbool.h>

static bool debugMode = false;

void toggleObstacleDebugMode() {
    debugMode = !debugMode;
}

void initObstacles(Obstacle obstacles[], int size) {
    if (size >= 23) { // Make sure the size is sufficient to hold all defined obstacles
        obstacles[0].bounds = (SDL_Rect){458, 48, 25, 25};
        obstacles[1].bounds = (SDL_Rect){475, 68, 25, 25};
        obstacles[2].bounds = (SDL_Rect){225, 545, 30, 30};
        obstacles[3].bounds = (SDL_Rect){247, 563, 26, 26};
        obstacles[4].bounds = (SDL_Rect){225, 567, 25, 25};
        obstacles[5].bounds = (SDL_Rect){132, 498, 22, 22};
        obstacles[6].bounds = (SDL_Rect){645, 332, 30, 30};
        obstacles[7].bounds = (SDL_Rect){712, 273, 50, 50};
        obstacles[8].bounds = (SDL_Rect){762, 270, 50, 50};
        obstacles[9].bounds = (SDL_Rect){762, 332, 50, 50};
        obstacles[10].bounds = (SDL_Rect){952, 457, 25, 25};
        obstacles[11].bounds = (SDL_Rect){978, 440, 29, 29};
        obstacles[12].bounds = (SDL_Rect){1185, 447, 50, 50};
        obstacles[13].bounds = (SDL_Rect){132, 610, 55, 55};
        obstacles[14].bounds = (SDL_Rect){75, 595, 55, 55};
        obstacles[15].bounds = (SDL_Rect){88, 643, 68, 45};
        obstacles[16].bounds = (SDL_Rect){145, 76, 53, 42};
        obstacles[17].bounds = (SDL_Rect){180, 106, 50, 40};
        obstacles[18].bounds = (SDL_Rect){220, 78, 28, 28};
        obstacles[19].bounds = (SDL_Rect){240, 105, 23, 23};
        obstacles[20].bounds = (SDL_Rect){255, 52, 23, 23};
        obstacles[21].bounds = (SDL_Rect){280, 38, 22, 22};
        obstacles[22].bounds = (SDL_Rect){250, 30, 30, 30};
    }
}

void drawObstacles(SDL_Renderer* renderer, Obstacle obstacles[], int size) {
    if (debugMode) {
        for (int i = 0; i < size; i++) {
            SDL_RenderDrawRect(renderer, &obstacles[i].bounds);
        }
        drawObstacleDebugInfo(renderer, obstacles, size);
    }
}

void drawObstacleDebugInfo(SDL_Renderer* renderer, Obstacle obstacles[], int size) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128); // Semi-transparent red for debug info
    for (int i = 0; i < size; i++) {
        SDL_RenderDrawRect(renderer, &obstacles[i].bounds);
    }
}