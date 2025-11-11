#ifndef POST_SDL_APP_H
#define POST_SDL_APP_H 1

#include "post/app.h"

PostError
PostSDLAppCreate(PostAppState** appState);

void
PostSDLAppLogInfo(PostAppState* appState, const char* fmt, va_list args);

void
PostSDLAppLogWarning(PostAppState* appState, const char* fmt, va_list args);

void
PostSDLAppDestroy(PostAppState* appState);

#endif
