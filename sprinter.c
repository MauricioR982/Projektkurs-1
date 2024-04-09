//
//  spaceman.c
//
//  Created by Jonas Willén on 2021-03-29.
//

#include "sprinter.h"

#define PUBLIC /* empty */
#define PRIVATE static


PRIVATE int WIDTH = 16;
PRIVATE int HEIGHT = 16;

struct Sprinter_type{
    int SPRINTER_POSITION_X;
    int SPRINTER_POSITION_Y;
    int SPRINTER_FRAME;
    int speed;
};

PUBLIC Sprinter createSprinterMan(int x, int y){
    Sprinter s = malloc(sizeof(struct Sprinter_type));
    s->SPRINTER_POSITION_Y = y;
    s->SPRINTER_POSITION_X = x;
    s->SPRINTER_FRAME = 0;
    s->speed = 1;
    return s;
}



PUBLIC int getSprinterHeight(){
    return HEIGHT;
}

PUBLIC int getSprinterWidth(){
    return WIDTH;
}

PUBLIC void tick(int direction){
    
    
}

