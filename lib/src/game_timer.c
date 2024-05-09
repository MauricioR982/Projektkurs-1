#include "game_timer.h"
#include <stdio.h>

static SDL_Texture* timerTexture = NULL;
static SDL_Rect timerRect;
static float remainingTime = 60.0f;
static char timeString[8];

void init_timer(SDL_Renderer* renderer, TTF_Font* font)
{
    sprintf(timeString, "1:00");
    SDL_Color color = {255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, timeString, color);
    timerTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    timerRect.x = (1280 - surface->w) / 2;
    timerRect.y = 50;
    timerRect.w = surface->w;
    timerRect.h = surface->h;
}

void update_timer(float deltaTime)
{
    if (remainingTime > 0)
    {
        remainingTime -= deltaTime;
        if (remainingTime <= 0)
        {
            remainingTime = 0;
            sprintf(timeString, "Game finished!");
        }
        else
        {
            int minutes = (int) remainingTime / 60;
            int seconds = (int) remainingTime % 60;
            sprintf(timeString, "%d:%02d", minutes, seconds);
        }
    }
}

void render_timer(SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_Color textColor = {50, 255, 50};
    SDL_Color backgroundColor = {0, 0, 255};

    if (timerTexture)
    {
        SDL_DestroyTexture(timerTexture);
    }

    SDL_Surface* surface = TTF_RenderText_Solid(font, timeString, textColor);
    timerTexture = SDL_CreateTextureFromSurface(renderer, surface);

    int padding = 10;
    SDL_Rect backgroundRect = {timerRect.x - padding, timerRect.y - padding, surface->w + 2 * padding, surface->h + 2 * padding};

    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);
    SDL_RenderFillRect(renderer, &backgroundRect);

    timerRect.w = surface->w;
    timerRect.h = surface->h;
    SDL_FreeSurface(surface);

    SDL_RenderCopy(renderer, timerTexture, NULL, &timerRect);
}

void cleanup_timer()
{
    if (timerTexture)
    {
        SDL_DestroyTexture(timerTexture);
        timerTexture = NULL;
    }
}