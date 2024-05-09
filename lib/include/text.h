#ifndef TEXT_H
#define TEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct text
{
    SDL_Renderer *pRenderer;
    SDL_Texture *pTexture;
    SDL_Rect rect;
    TTF_Font *pFont;
    SDL_Color color;
} Text;

Text *createText(SDL_Renderer *pRenderer, int r, int g, int b, TTF_Font *pFont, char *pString, int x, int y);
void drawText(Text *pText);
void destroyText(Text *pText);
void updateText(Text *pText, SDL_Renderer *pRenderer, const char *newText);

#endif //TEXT_H