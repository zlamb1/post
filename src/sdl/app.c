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

#include <SDL3/SDL_log.h>
#include <stdio.h>
#include <stdlib.h>

#include "post/app.h"
#include "post/compiler.h"
#include "post/config.h"
#include "post/error.h"
#include "post/font.h"
#include "post/parser.h"
#include "post/proc.h"

#include "post/sdl/app.h"
#include "post/sdl/log.h"
#include "post/sdl/renderer.h"

#define WIDTH  500
#define HEIGHT 500

extern char** environ;

PostError
PostSDLAppCreate(PostAppState** appState)
{
  PostError        error     = POST_ERR_OUT_OF_MEMORY;
  PostAppState*    _appState = malloc(sizeof(PostAppState));
  PostSDLRenderer* renderer  = malloc(sizeof(PostSDLRenderer));

  if (_appState == NULL || renderer == NULL)
    goto fail;

  memset(_appState, 0, sizeof(PostAppState));

  PostLoadConfig(&_appState->config);

  _appState->parser.state = POST_PARSER_STATE_NORMAL;

  _appState->cursor.fg = _appState->config.fg;
  _appState->cursor.bg = _appState->config.bg;

  _appState->renderer = (PostRenderer*) renderer;

  _appState->LogInfo    = PostSDLAppLogInfo;
  _appState->LogWarning = PostSDLAppLogWarning;
  _appState->DestroyApp = PostSDLAppDestroy;

  memset(renderer, 0, sizeof(PostSDLRenderer));

  renderer->base.windowWidth  = WIDTH;
  renderer->base.windowHeight = HEIGHT;

  renderer->base.SetWindowTitle = PostSDLSetTitle;
  renderer->base.RenderFrame    = PostSDLRenderFrame;

  error = PostChildProcessSpawn(
    "/bin/bash", &_appState->master, &_appState->childProcess);

  if (error != POST_ERR_NONE)
    goto fail;

  if (PostFontSystemInit()) {
    error = POST_ERR_SUBSYS;
    goto fail;
  }

  error = PostFontCreate(&renderer->activeFont);

  if (error != POST_ERR_NONE)
    goto fail;

  PostFontSetSize(&renderer->activeFont, 20);

  error = PostSDLSetCellSize(renderer);

  if (error != POST_ERR_NONE)
    goto fail;

  renderer->glyphBitmap =
    malloc(renderer->base.cellWidth * renderer->base.cellHeight);

  if (renderer->glyphBitmap == NULL) {
    error = POST_ERR_OUT_OF_MEMORY;
    goto fail;
  }

  error = PostAppSizeGrid(_appState);

  if (error != POST_ERR_NONE)
    goto fail;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    error = POST_ERR_SUBSYS;
    PostLogErrorA("Could Not Initialize SDL Video: %s", SDL_GetError());
    goto fail;
  }

  if (!SDL_CreateWindowAndRenderer("Post",
                                   WIDTH,
                                   HEIGHT,
                                   SDL_WINDOW_RESIZABLE,
                                   &renderer->sdlWindow,
                                   &renderer->sdlRenderer)) {
    error = POST_ERR_SUBSYS;
    PostLogErrorA("Could Not Create Window And Renderer: %s", SDL_GetError());
    goto fail;
  }

  SDL_SetRenderLogicalPresentation(
    renderer->sdlRenderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
  SDL_SetRenderDrawBlendMode(renderer->sdlRenderer, SDL_BLENDMODE_BLEND);
  SDL_StartTextInput(renderer->sdlWindow);

  *appState = _appState;

  return POST_ERR_NONE;

fail:
  if (renderer != NULL) {
    if (renderer->glyphBitmap != NULL)
      free(renderer->glyphBitmap);
    free(renderer);
  }

  if (_appState != NULL)
    free(_appState);

  return error;
}

void
PostSDLAppLogInfo(UNUSED PostAppState* appState, const char* fmt, va_list args)
{
  SDL_LogMessageV(
    SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
}

void
PostSDLAppLogWarning(UNUSED PostAppState* appState,
                     const char*          fmt,
                     va_list              args)
{
  SDL_LogMessageV(
    SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, args);
}

void
PostSDLAppDestroy(PostAppState* appState)
{
  if (appState->childProcess != NULL)
    PostProcessDestroy(appState->childProcess);

  if (appState->master != NULL)
    fclose(appState->master);

  PostFontSystemFini();
  free(appState);
}
