#include "hunter.h"

struct Hunter_type {
    int positionX, positionY;
    int speed;
};

Hunter createHunterMan(int x, int y) {
    Hunter h = malloc(sizeof(struct Hunter_type));
    h->positionX = x;
    h->positionY = y;
    h->speed = 1; 
    return h;
}

int getHunterPositionX(Hunter h) {
    return h->positionX;
}

int getHunterPositionY(Hunter h) {
    return h->positionY;
}

