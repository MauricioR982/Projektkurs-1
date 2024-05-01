// game_data.h
#ifndef GAME_DATA_H
#define GAME_DATA_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_PLAYERS 4

#define HUNTER 0
#define SPRINTER 1

enum gameState{
    STATE_MENU,
    STATE_START_GAME,
    STATE_TUTORIAL,
    STATE_TOTAL,
    STATE_CONNECTING,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_EXIT
}; typedef enum gameState GameState;


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

typedef struct {
    int playerId;          // Unique identifier for each player
    SDL_Rect position;
    SDL_Texture *texture;
    SDL_Rect spriteClips[8];
    SDL_RendererFlip flip;
    int currentFrame;
    int isActive;          // Indicates if the player is active in the game
    int type;              // Type of the player (HUNTER or SPRINTER)
} Player;

typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;

#endif // GAME_DATA_H
