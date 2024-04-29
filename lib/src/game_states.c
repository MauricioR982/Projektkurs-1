// game_states.c
#include "game_states.h"

// Static variable - local to this file, provides encapsulation
static GameState current_state;

void initializeGameState() {
    current_state = STATE_CONNECTING;  // Initial state
}

void setGameState(GameState new_state) {
    current_state = new_state;
}

GameState getGameState() {
    return current_state;
}
