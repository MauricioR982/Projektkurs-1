#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../../lib/include/text.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

Text *createText(SDL_Renderer *pRenderer, int r, int g, int b, TTF_Font *pFont, char *pString, int x, int y){
    Text *pText = malloc(sizeof(struct text));
    pText->pRenderer = pRenderer;
    SDL_Color color = {r,g,b};
    SDL_Surface *pSurface = TTF_RenderText_Solid(pFont,pString,color);
    if(!pSurface){
        printf("Error: %s\n",SDL_GetError());
        return NULL;
    }
    pText->pTexture = SDL_CreateTextureFromSurface(pRenderer,pSurface);
    SDL_FreeSurface(pSurface);
    if(!pText->pTexture){
        printf("Error: %s\n",SDL_GetError());
        return NULL;
    }
    
    SDL_QueryTexture(pText->pTexture,NULL,NULL,&pText->rect.w,&pText->rect.h);
    pText->rect.x = x - pText->rect.w/2;
    pText->rect.y = y - pText->rect.h/2;
    return pText;
}

void drawText(Text *pText){
    SDL_RenderCopy(pText->pRenderer,pText->pTexture,NULL,&pText->rect);
}

void destroyText(Text *pText){
    SDL_DestroyTexture(pText->pTexture);
    free(pText);
}

void updateText(Text *pText, SDL_Renderer *pRenderer, const char *newString) {
    if (!pText || !newString) return;

    // Destroy the old texture
    if (pText->pTexture) {
        SDL_DestroyTexture(pText->pTexture);
    }

    // Create a new surface and texture with the updated text
    SDL_Color color = {255, 255, 255}; // Adjust to your desired color
    TTF_Font *font = TTF_OpenFont("../lib/resources/arial.ttf", 40); // Adjust the path and size as needed
    if (!font) {
        printf("Error: %s\n", TTF_GetError());
        return;
    }
    SDL_Surface *surface = TTF_RenderText_Solid(font, newString, color);
    if (!surface) {
        printf("Error: %s\n", SDL_GetError());
        return;
    }
    pText->pTexture = SDL_CreateTextureFromSurface(pRenderer, surface);
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);

    // Update the text rect dimensions
    SDL_QueryTexture(pText->pTexture, NULL, NULL, &pText->rect.w, &pText->rect.h);
    
    // Center horizontally while keeping the same vertical position
    pText->rect.x = (WINDOW_WIDTH - pText->rect.w) / 2;
    // Adjust to an appropriate vertical position, e.g., 30px from the top
    pText->rect.y = 30;
}

