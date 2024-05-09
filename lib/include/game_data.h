#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_PLAYERS 2

#define HUNTER 0
#define SPRINTER 1
#define HORIZONTAL_MARGIN 20 // Margin for character movement constraints
#define NUM_OBSTACLES 23     // Number of obstacles

// Enumerations for different game states
typedef enum {
    GAME_MENU, //Game menu
    GAME_TUTORIAL, //Game tutorial
    GAME_WAITING, // Waiting for players to be ready
    GAME_READY,   // Player has signaled readiness
    GAME_START,   // Start the game
    GAME_ONGOING, // Game is in progress
    GAME_OVER     // Game has ended
} GameState;

// Commands that can be sent from the client to the server
typedef enum {
    CMD_READY, // Command to signal readiness to start the game
    CMD_RUN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_UP,
    CMD_DOWN
} ClientCommand;

// Structure for client commands
typedef struct {
    ClientCommand command; // Command type
    int playerNumber;      // Which player is sending the command
} ClientData;

// Enumeration for player roles
typedef enum {
    ROLE_SPRINTER,
    ROLE_HUNTER
} PlayerRole;

// Structure representing a player's position, size, and role
typedef struct {
    float x, y, w, h;
    PlayerRole role; // Role of the player (hunter or sprinter)
    int score;
} PlayerData;

// Data structure sent from the server to the clients
typedef struct {
    PlayerData players[MAX_PLAYERS]; // Player data for all players
    int playerNr;                    // Index of the player to which the data is being sent
    GameState state;                 // Current state of the game
} ServerData;

// Structure for each player in the game
typedef struct {
    int playerId;          // Unique identifier for each player
    SDL_Rect position;     // Position and size of the player
    SDL_Texture *texture;  // Texture used for rendering the player
    SDL_Rect spriteClips[8]; // Animation frames for the player
    SDL_RendererFlip flip; // Rendering flip state
    int currentFrame;      // Current frame of animation
    int isActive;          // Indicates if the player is active
    int type;              // Type of the player (HUNTER or SPRINTER)
    int positionX, positionY;
    int speed;
    int score;
    int role;
} Player;

// Structure for transmitting player movement information
typedef struct {
    int playerId; // Player ID
    int x, y;     // New coordinates of the player
} PlayerMovement;

SDL_Point sprinterSpawnPoints[] = {
    {100, 64},   // First sprinter position
    {100, 550},  // Second sprinter position
    {1100, 64}   // Third sprinter position
};

#endif // GAME_DATA_H
