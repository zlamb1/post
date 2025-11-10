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

#include "post/app.h"
#include "post/font.h"
#include "post/proc.h"

#include "post/sdl/renderer.h"

PostError
PostSDLSetCellSize(PostSDLRenderer* renderer)
{
  PostFont         font = renderer->activeFont;
  PostGlyphMetrics spaceMetrics;

  PostTry(PostFontGetGlyphMetrics(&font, 0x20, &spaceMetrics));

  renderer->base.cellWidth  = spaceMetrics.horizontalAdvance;
  renderer->base.cellHeight = font.height;

  return POST_ERR_NONE;
}

PostError
PostSDLSetTitle(PostAppState* appState, const char* title)
{
  PostSDLRenderer* renderer = (PostSDLRenderer*) appState->renderer;
  SDL_SetWindowTitle(renderer->sdlWindow, title);
  return POST_ERR_NONE;
}

PostError
PostSDLRenderFrame(PostAppState* appState)
{
  PostSDLRenderer* renderer    = (PostSDLRenderer*) appState->renderer;
  SDL_Renderer*    sdlRenderer = renderer->sdlRenderer;
  PostFont         font        = renderer->activeFont;
  puint8*          glyphBitmap = renderer->glyphBitmap;

  PostCursor   cursor     = appState->cursor;
  PostCellGrid grid       = appState->grid;
  puint32      cellWidth  = renderer->base.cellWidth;
  puint32      cellHeight = renderer->base.cellHeight;

  PostChildProcessPoll(appState);

  SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
  SDL_RenderClear(sdlRenderer);

  Uint64 ticks = SDL_GetTicks();
  if (ticks - cursor.time > 500) {
    appState->cursor.visible = !cursor.visible;
    appState->cursor.time    = ticks;
  }

  for (puint32 cy = 0; cy < grid.height; ++cy) {
    for (puint32 cx = 0; cx < grid.width; ++cx) {
      PostCell cell = grid.cells[cy * grid.width + cx];

      if (!cell.charCode)
        continue;

      PostError error = PostFontLoadGlyph(
        &font, cell.charCode, cellWidth, cellHeight, cellWidth, glyphBitmap);

      if (error != POST_ERR_NONE)
        return error;

      puint32 rx = cx * cellWidth;
      puint32 ry = cy * cellHeight;

      SDL_SetRenderDrawColor(
        sdlRenderer, cell.bg.r, cell.bg.g, cell.bg.b, cell.bg.a);
      SDL_FRect cellRect =
        (SDL_FRect) { .x = rx, .y = ry, .w = cellWidth, .h = cellHeight };
      SDL_RenderFillRect(sdlRenderer, &cellRect);

      if (cell.sgr & POST_CELL_SGR_UNDERLINE) {
        SDL_SetRenderDrawColor(
          sdlRenderer, cell.fg.r, cell.fg.g, cell.fg.b, cell.fg.a);
        SDL_RenderLine(sdlRenderer,
                       rx,
                       ry + font.ascender + 2,
                       rx + cellWidth,
                       ry + font.ascender + 2);
      }

      for (puint32 y = 0; y < cellHeight; ++y) {
        for (puint32 x = 0; x < cellWidth; ++x) {
          puint8 alpha = glyphBitmap[y * cellWidth + x];

          if (alpha) {
            SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
            SDL_RenderPoint(sdlRenderer, rx + x, ry + y);

            SDL_SetRenderDrawColor(sdlRenderer,
                                   cell.fg.r,
                                   cell.fg.g,
                                   cell.fg.b,
                                   cell.fg.a * (alpha / 255.0));
            SDL_RenderPoint(sdlRenderer, rx + x, ry + y);
          }
        }
      }
    }
  }

  if (cursor.visible) {
    SDL_SetRenderDrawColor(
      sdlRenderer, cursor.fg.r, cursor.fg.g, cursor.fg.b, cursor.fg.a);
    SDL_RenderLine(sdlRenderer,
                   cursor.x * cellWidth,
                   cursor.y * cellHeight,
                   cursor.x * cellWidth,
                   cursor.y * cellHeight + cellHeight);
  }

  SDL_RenderPresent(sdlRenderer);

  return POST_ERR_NONE;
}
