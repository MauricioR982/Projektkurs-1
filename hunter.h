//
//  hunter.h
//
// 
//

#ifndef hunter_h
#define hunter_h

#include <stdio.h>
#include <stdlib.h>

typedef struct Hunter_type *Hunter;

int getHunterPositionX(Hunter h);
int getHunterPositionY(Hunter h);
Hunter createHunterMan(int x, int y);

#endif /* hunter_h */



