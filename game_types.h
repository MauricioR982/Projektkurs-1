//game_types.h
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <SDL2/SDL.h>

typedef struct {
    int x, y; // position
    int w, h; // dimensions
    bool active; // is the player currently active
} Player;


#endif
