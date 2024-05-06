#include "hunter.h"

struct Hunter_type {
    int positionX, positionY;
    int speed;
    // Other hunter-specific properties
};

Hunter createHunterMan(int x, int y) {
    Hunter h = malloc(sizeof(struct Hunter_type));
    h->positionX = x;
    h->positionY = y;
    h->speed = 1; // Example speed value
    return h;
}

int getHunterPositionX(Hunter h) {
    return h->positionX;
}

int getHunterPositionY(Hunter h) {
    return h->positionY;
}

