/*
 * Copyright (c) 2025 Zachary Lamb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "post/proc.h"
#include "post/sdl/app.h"

SDL_AppResult
SDL_AppInit(void** appstate, int argc, char* argv[])
{
  PostError error;

  (void) argc;
  (void) argv;

  error = PostSDLAppCreate((PostAppState**) appstate);

  if (error != POST_ERR_NONE) {
    if (error != POST_ERR_SUBSYS)
      SDL_LogError(
        SDL_LOG_CATEGORY_ERROR, "App Error: %s", PostErrorString(error));
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppEvent(void* appstate, SDL_Event* event)
{
  PostAppState* appState;

  if (appstate == NULL)
    return SDL_APP_FAILURE;

  appState = (PostAppState*) appstate;

  switch (event->type) {
    case SDL_EVENT_KEY_DOWN: {
      SDL_Keycode keyCode   = event->key.key;
      const char* str       = NULL;
      const bool* keyStates = SDL_GetKeyboardState(NULL);

      switch (keyCode) {
        case SDLK_BACKSPACE:
          str = "\b";
          break;
        case SDLK_RETURN:
          str = "\n";
          break;
        case SDLK_LEFT:
          str = "\x1b[D";
          break;
        case SDLK_RIGHT:
          str = "\x1b[C";
          break;
        case SDLK_UP:
          str = "\x1b[A";
          break;
        case SDLK_DOWN:
          str = "\x1b[B";
          break;
        case SDLK_C:
          if (keyStates[SDL_SCANCODE_LCTRL] || keyStates[SDL_SCANCODE_RCTRL])
            str = "\x3";
          break;
        case SDLK_Z:
          if (keyStates[SDL_SCANCODE_LCTRL] || keyStates[SDL_SCANCODE_RCTRL])
            str = "\x1A";
          break;
      }

      if (str != NULL)
        PostChildProcessSend(appState, str, strlen(str));

      break;
    }
    case SDL_EVENT_TEXT_INPUT: {
      const char* text = event->text.text;
      PostChildProcessSend(appState, text, strlen(text));
      break;
    }
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppIterate(void* appstate)
{
  PostError     error;
  PostAppState* appState;

  if (appstate == NULL)
    return SDL_APP_FAILURE;

  appState = (PostAppState*) appstate;
  error    = appState->renderer->RenderFrame(appState);

  if (error != POST_ERR_NONE) {
    SDL_LogError(SDL_LOG_PRIORITY_ERROR,
                 "Error Rendering Frame: %s",
                 PostErrorString(error));
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

void
SDL_AppQuit(void* appstate, SDL_AppResult appresult)
{
  (void) appresult;
  PostAppDestroy(appstate);
}
