#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_PLAYERS 3
#define MAX_PERKS 4

#define HUNTER 0
#define SPRINTER 1
#define HORIZONTAL_MARGIN 20 
#define NUM_OBSTACLES 23     

typedef enum {
    GAME_MENU, 
    GAME_TUTORIAL, 
    GAME_WAITING, 
    GAME_READY,   
    GAME_START,   
    GAME_ONGOING, 
    GAME_OVER,     
    GAME_ENTER_IP 
} GameState;

typedef enum {
    CMD_READY, 
    CMD_RUN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_UP,
    CMD_DOWN,
    CMD_RESET
} ClientCommand;


typedef struct {
    ClientCommand command; 
    int playerNumber;      
    int seqNum;            
    bool ack;              
} ClientData;


typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;


typedef struct {
    float x, y, w, h;
    PlayerRole role; 
    int currentFrame;
    SDL_RendererFlip flip;
} PlayerData;

typedef struct {
    int type;
    SDL_Rect position;
    int duration;
    bool active;
    Uint32 startTime;
    int perkSpawnTimer;
    int perkSpawnInterval;
    int dx;
    int dy;
} Perk;


typedef struct {
    PlayerData players[MAX_PLAYERS]; 
    int playerNr; 
    GameState state; 
    int remainingTime;
    Perk perks[MAX_PERKS];
    int seqNum; 
    bool ack; 
} ServerData;


typedef struct {
    int playerId;
    SDL_Rect position; 
    SDL_Texture *texture; 
    SDL_Rect spriteClips[8];
    SDL_RendererFlip flip; 
    int currentFrame; 
    int isActive; 
    int type; 
    float speed; 
    float originalSpeed; 
    Uint32 perkStartTime; 
    int activePerkType;
} Player;


typedef struct {
    int playerId; 
    int x, y; 
} PlayerMovement;


SDL_Point sprinterSpawnPoints[] = {
    {100, 64},
    {100, 550},
    {1100, 64}
};

#endif // GAME_DATA_H
