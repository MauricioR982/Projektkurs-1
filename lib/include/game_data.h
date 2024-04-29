// game_data.h
#ifndef GAME_DATA_H
#define GAME_DATA_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_PLAYERS 2

/*typedef enum {
    STATE_MENU,
    STATE_START_GAME,
    STATE_TUTORIAL,
    STATE_TOTAL,
    STATE_CONNECTING,
    STATE_LOBBY,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_EXIT
} GameState;*/

enum gameState{START, ONGOING, GAME_OVER};
typedef enum gameState GameState;

enum clientCommand{READY, RUN, LEFT, RIGHT, UP, DOWN};
typedef enum clientCommand ClientCommand;

struct clientData{
    ClientCommand command;
    int playerNumber;
};
typedef struct clientData ClientData;

struct playerData{
    float x, y, h, w;    
};
typedef struct playerData playerData;   

struct serverData{
    playerData players[MAX_PLAYERS];
    int playerNr;
    GameState gState;
};
typedef struct serverData ServerData;


#endif // GAME_DATA_H
