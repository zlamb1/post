#ifndef POST_SDL_APP_H
#define POST_SDL_APP_H 1

#include "post/app.h"

PostError
PostSDLAppCreate(PostAppState** appState);

void
PostSDLAppDestroy(PostAppState* appState);

#endif
