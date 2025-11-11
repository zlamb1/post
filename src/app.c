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

#include <stdlib.h>

#include "post.h"
#include "post/parser.h"
#include "post/string.h"
#include "post/unicode.h"

static puint32
PostAppAdvanceY(PostCellGrid grid, puint32 y)
{
  if (++y == grid.height) {
    puint32 width = grid.width;
    y             = grid.height - 1;
    memmove(grid.cells, grid.cells + width, width * y * sizeof(PostCell));
    memset(grid.cells + width * y, 0, grid.width * sizeof(PostCell));
  }

  return y;
}

static void
PostAppAdvance(PostCellGrid grid, PostCursor* cursor)
{
  if (++cursor->x == grid.width) {
    if (cursor->lastColumnFlag) {
      cursor->lastColumnFlag = 0;
      cursor->x              = 0;
      cursor->y              = PostAppAdvanceY(grid, cursor->y);
    } else {
      cursor->lastColumnFlag = 1;
      cursor->x              = grid.width - 1;
    }
  }
}

void
PostAppWriteASCIIString(PostAppState* appState, const char* str)
{
  PostParser* parser = &appState->parser;
  PostCursor  cursor = appState->cursor;
  char        ch;

ParserLoop:
  ch = str[0];

  if (!ch)
    goto AssignCursor;

  switch (appState->parser.state) {
    case POST_PARSER_STATE_NORMAL:
      break;
    case POST_PARSER_STATE_ESC:
      switch (ch) {
        case POST_UNICODE_LBRACK:
          parser->state         = POST_PARSER_STATE_CSI;
          parser->isPrivate     = 0;
          parser->attribs       = NULL;
          parser->currentAttrib = NULL;
          ++str;

          if (str[0] == POST_UNICODE_QUESTION_MARK) {
            parser->isPrivate = 1;
            ++str;
          }

          break;
        case POST_UNICODE_RBRACK:
          parser->state     = POST_PARSER_STATE_OSC;
          parser->parseText = 0;
          parser->s         = 255;
          parser->t         = (PostString) { 0 };
          ++str;
          break;
        case POST_UNICODE_LPAREN:
          parser->state = POST_PARSER_STATE_DESIGNATE_G0;
          ++str;
          break;
        case POST_UNICODE_E: // NEL
          cursor.lastColumnFlag = 0;
          cursor.x              = 0;
          cursor.y              = PostAppAdvanceY(appState->grid, cursor.y);
          break;
        default:
          parser->state = POST_PARSER_STATE_NORMAL;
          ++str;
          // FIXME: create warn logging function
          printf("unexpected char after escape: '%c'\n", ch);
          break;
      }

      goto ParserLoop;
    case POST_PARSER_STATE_DESIGNATE_G0:
      parser->state = POST_PARSER_STATE_NORMAL;
      ++str;
      goto ParserLoop;
    case POST_PARSER_STATE_CSI:
      PostParseCSI(appState, &cursor, ch);
      ++str;
      goto ParserLoop;
    case POST_PARSER_STATE_OSC:
      if (parser->parseText) {
        PostError error;

        if (ch == POST_UNICODE_BEL) {
          error = PostStringAppendChar(&parser->t, 0);
          if (error != POST_ERR_NONE)
            printf("WARNING: OSC Failed: %s\n", PostErrorString(error));
          else
            PostAppSetTitle(appState, parser->t.buf);
          PostStringRelease(&parser->t);
          parser->state = POST_PARSER_STATE_NORMAL;
        } else {
          error = PostStringAppendChar(&parser->t, ch);
          if (error != POST_ERR_NONE)
            printf("WARNING: OSC Failed: %s\n", PostErrorString(error));
        }
      } else {
        if (parser->s != 255) {
          if (ch != POST_UNICODE_SEMICOLON) {
            parser->state = POST_PARSER_STATE_NORMAL;
            printf("WARNING: invalid OSC: expected ';' -> got '%c'\n", ch);
            goto ParserLoop;
          }
          parser->parseText = 1;
          ++str;
          goto ParserLoop;
        }

        switch (ch) {
          case POST_UNICODE_0:
          case POST_UNICODE_1:
          case POST_UNICODE_2:
            parser->s = ch - POST_UNICODE_0;
            break;
          default:
            parser->state = POST_PARSER_STATE_NORMAL;
            printf("WARNING: invalid OSC: '%c'\n", ch);
            break;
        }
      }

      ++str;
      goto ParserLoop;
  }

  for (; str[0]; ++str) {
    switch (str[0]) {
      case POST_UNICODE_BEL:
        continue;
      case POST_UNICODE_BS:
        cursor.lastColumnFlag = 0;
        if (cursor.x)
          --cursor.x;
        else if (cursor.y) {
          cursor.x = appState->grid.width - 1;
          --cursor.y;
        }
        continue;
      case POST_UNICODE_HT:
        cursor.lastColumnFlag = 0;
        for (int i = 0; i < appState->config.tabWidth; ++i)
          PostAppAdvance(appState->grid, &cursor);
        continue;
      case POST_UNICODE_LF:
      case POST_UNICODE_FF:
        cursor.lastColumnFlag = 0;
        cursor.x              = 0;
        cursor.y              = PostAppAdvanceY(appState->grid, cursor.y);
        continue;
      case POST_UNICODE_CR:
        cursor.lastColumnFlag = 0;
        cursor.x              = 0;
        continue;
      case POST_UNICODE_ESC:
        parser->state = POST_PARSER_STATE_ESC;
        ++str;
        goto ParserLoop;
      case POST_UNICODE_SUB:
        goto ParserLoop;
      default:
        break;
    }

    if (cursor.lastColumnFlag) {
      cursor.lastColumnFlag = 0;
      cursor.x              = 0;
      cursor.y              = PostAppAdvanceY(appState->grid, cursor.y);
    }

    appState->grid.cells[cursor.y * appState->grid.width + cursor.x] =
      (PostCell) {
        .charCode = (unsigned char) str[0],
        .fg       = cursor.fg,
        .bg       = cursor.bg,
        .sgr      = cursor.sgr,
      };

    PostAppAdvance(appState->grid, &cursor);
  }

AssignCursor:
  appState->cursor = cursor;
}

PostError
PostAppSizeGrid(PostAppState* appState)
{
  PostRenderer* renderer = appState->renderer;
  pusize        byteSize;
  puint32       gridWidth  = renderer->windowWidth / renderer->cellWidth;
  puint32       gridHeight = renderer->windowHeight / renderer->cellHeight;

  if (!gridWidth)
    gridWidth = 1;

  if (!gridHeight)
    gridHeight = 1;

  byteSize = gridWidth * gridHeight * sizeof(PostCell);

  if (byteSize > appState->grid.byteSize) {
    PostCell* cells = realloc(appState->grid.cells, byteSize);
    if (cells == NULL)
      return POST_ERR_OUT_OF_MEMORY;
    memset(cells, 0, byteSize);
    appState->grid.byteSize = byteSize;
    appState->grid.cells    = cells;
  }

  appState->grid.width  = gridWidth;
  appState->grid.height = gridHeight;

  return POST_ERR_NONE;
}
