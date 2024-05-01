#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_PLAYERS 4

#define HUNTER 0
#define SPRINTER 1
#define HORIZONTAL_MARGIN 20 // Margin for character movement constraints
#define NUM_OBSTACLES 23 // Number of obstacles if this is constant

// Game state enumeration
enum gameState {
    STATE_MENU,
    STATE_START_GAME,
    STATE_TUTORIAL,
    STATE_TOTAL,
    STATE_CONNECTING,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_EXIT
};
typedef enum gameState GameState;

// Commands that can be sent from the client to the server
enum clientCommand {
    READY,
    RUN,
    LEFT,
    RIGHT,
    UP,
    DOWN
};
typedef enum clientCommand ClientCommand;

// Data structure for client commands
struct clientData {
    ClientCommand command;
    int playerNumber;
};
typedef struct clientData ClientData;

// Data structure representing the position and size of a player
struct playerData {
    float x, y, h, w;    
};
typedef struct playerData PlayerData;

// Data sent from the server to the clients
struct serverData {
    PlayerData players[MAX_PLAYERS];
    int playerNr;
    GameState gState;
};
typedef struct serverData ServerData;

// Player structure used in the game
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

// Player roles
typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;

// Structure for transmitting player movement
struct playerMovement {
    int playerId;
    int x;
    int y;
};
typedef struct playerMovement PlayerMovement;

#endif // GAME_DATA_H
