#ifndef GAME_TIMER_H
#define GAME_TIMER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void init_timer(SDL_Renderer* renderer, TTF_Font* font);
void update_timer(float deltaTime);
void render_timer(SDL_Renderer* renderer, TTF_Font* font);
void cleanup_timer();

#endif // GAME_TIMER_H