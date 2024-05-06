#include "sprinter.h"

struct Sprinter_type {
    int positionX, positionY;
    int speed;
};

Sprinter createSprinterMan(int x, int y) {
    Sprinter s = malloc(sizeof(struct Sprinter_type));
    s->positionX = x;
    s->positionY = y;
    s->speed = 2;
    return s;
}

int getSprinterPositionX(Sprinter s) {
    return s->positionX;
}

int getSprinterPositionY(Sprinter s) {
    return s->positionY;
}