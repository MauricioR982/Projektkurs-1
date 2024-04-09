//
//  sprinter.h
//
// 
//

#ifndef sprinter_h
#define sprinter_h

#include <stdio.h>
#include <stdlib.h>

typedef struct Sprinter_type *Sprinter;

int getSprinterPositionX(Sprinter s);
int getSprinterPositionY(Sprinter s);
Sprinter createSprinterMan(int x, int y);

#endif /* sprinter_h */



