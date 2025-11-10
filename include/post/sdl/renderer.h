#ifndef POST_SDL_RENDERER_H
#define POST_SDL_RENDERER_H 1

#include <SDL3/SDL.h>

#include "post/font.h"
#include "post/renderer.h"
#include "post/types.h"

typedef struct
{
  PostRenderer  base;
  SDL_Window*   sdlWindow;
  SDL_Renderer* sdlRenderer;
  PostFont      activeFont;
  puint8*       glyphBitmap;
} PostSDLRenderer;

PostError
PostSDLSetCellSize(PostSDLRenderer* renderer);

PostError
PostSDLSetTitle(PostAppState* appState, const char* title);

PostError
PostSDLRenderFrame(PostAppState* appState);

#endif
