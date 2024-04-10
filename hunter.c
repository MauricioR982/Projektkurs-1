//
//  hunter.c
//
//

#include "hunter.h"

#define PUBLIC /* empty */
#define PRIVATE static


PRIVATE int WIDTH = 16;
PRIVATE int HEIGHT = 16;

struct Hunter_type{
    int HUNTER_POSITION_X;
    int HUNTER_POSITION_Y;
    int HUNTER_FRAME;
    int speed;
};

PUBLIC Hunter createHunterMan(int x, int y){
    Hunter h = malloc(sizeof(struct Hunter_type));
    h->HUNTER_POSITION_Y = y;
    h->HUNTER_POSITION_X = x;
    h->HUNTER_FRAME = 0;
    h->speed = 1;
    return h;
}



PUBLIC int getHunterHeight(){
    return HEIGHT;
}

PUBLIC int getHunterWidth(){
    return WIDTH;
}

PUBLIC void tick(int direction){
    
    
}

