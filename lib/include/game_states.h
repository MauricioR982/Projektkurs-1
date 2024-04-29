// game_states.h
#ifndef GAME_STATES_H
#define GAME_STATES_H

typedef enum {
    STATE_MENU,
    STATE_START_GAME,
    STATE_TUTORIAL,
    STATE_TOTAL,
    STATE_CONNECTING,
    STATE_LOBBY,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_EXIT
} GameState;

void initializeGameState();
void setGameState(GameState new_state);
GameState getGameState();


#endif // GAME_STATES_H
