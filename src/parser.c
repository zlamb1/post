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

// match xterm colors
static PostColor sgrColors[16] = {
  PostColorRGB(0, 0, 0),       PostColorRGB(205, 0, 0),
  PostColorRGB(0, 205, 0),     PostColorRGB(205, 205, 0),
  PostColorRGB(0, 0, 238),     PostColorRGB(205, 0, 205),
  PostColorRGB(0, 205, 205),   PostColorRGB(229, 229, 229),
  PostColorRGB(127, 127, 127), PostColorRGB(255, 0, 0),
  PostColorRGB(0, 255, 0),     PostColorRGB(255, 255, 0),
  PostColorRGB(92, 92, 255),   PostColorRGB(255, 0, 255),
  PostColorRGB(0, 255, 255),   PostColorRGB(255, 255, 255),
};

#define PostAttributesIterate(ATTRIBS)                                         \
  for (PostAttribute* current = (ATTRIBS),                                     \
                      *tmp    = ((void) (current != NULL &&                    \
                                      ((current = current->prev),           \
                                       ((ATTRIBS)->prev = NULL))),          \
                              current);                                     \
       current != NULL;                                                        \
       current = current->prev, tmp = (free(tmp), current))

#define PostGetCell(X, Y) appState->grid.cells[(Y) * gwidth + (X)]

static inline void
PostParseDECSET(PostAppState* appState)
{
  PostAttribute* attribs = appState->parser.attribs;

  if (attribs == NULL)
    printf("WARNING: expected DECSET value\n");
  else {
    PostAttributesIterate(attribs)
    {
      switch (current->n) {
        case 2004:
          appState->config.bracketed_paste_mode = 1;
          break;
        default:
          printf("WARNING: unknown DECSET value: '%u'\n", current->n);
          break;
      }
    }
  }
}

static inline void
PostParseEL(PostAppState* appState, PostCursor* cursor)
{
  PostAttribute* attribs = appState->parser.attribs;

  if (attribs == NULL) {
    for (puint32 x = cursor->x; x < appState->grid.width; ++x)
      appState->grid.cells[cursor->y * appState->grid.width + x].charCode = 0;
  } else {
    puint32 start, end;

    PostAttributesIterate(attribs)
    {
      switch (current->n) {
        case 0:
          start = cursor->x;
          end   = appState->grid.width;
          break;
        case 1:
          start = 0;
          end   = cursor->x + 1;
          break;
        case 2:
          start = 0;
          end   = appState->grid.width;
          break;
        default:
          printf("WARNING: Invalid Erase in Line Value: '%u'\n", current->n);
          continue;
      }

      for (puint32 x = start; x < end; ++x)
        appState->grid.cells[cursor->y * appState->grid.width + cursor->x]
          .charCode = 0;
    }
  }
}

static inline void
PostParseSGR(PostAppState* appState, PostCursor* cursor)
{
  PostAttribute* attribs = appState->parser.attribs;

  if (attribs == NULL) {
    // NOTE: default to resetting cursor attributes
    cursor->sgr = 0;
  } else {
    PostAttributesIterate(attribs)
    {
      switch (current->n) {
        case 0:
          cursor->fg  = appState->config.fg;
          cursor->bg  = appState->config.bg;
          cursor->sgr = 0;
          break;
        case 1:
          cursor->sgr &= ~POST_CELL_SGR_FAINT;
          cursor->sgr |= POST_CELL_SGR_BOLD;
          break;
        case 2:
          cursor->sgr &= ~POST_CELL_SGR_BOLD;
          cursor->sgr |= POST_CELL_SGR_FAINT;
          break;
        case 3:
          cursor->sgr |= POST_CELL_SGR_ITALIC;
          break;
        case 4:
          cursor->sgr &= ~POST_CELL_SGR_DBL_UNDERLINE;
          cursor->sgr |= POST_CELL_SGR_UNDERLINE;
          break;
        case 5:
          cursor->sgr &= ~POST_CELL_SGR_RAPID_BLINK;
          cursor->sgr |= POST_CELL_SGR_SLOW_BLINK;
          break;
        case 6:
          cursor->sgr &= ~POST_CELL_SGR_SLOW_BLINK;
          cursor->sgr |= POST_CELL_SGR_RAPID_BLINK;
          break;
        case 7:
          cursor->sgr |= POST_CELL_SGR_INVERT;
          break;
        case 8:
          cursor->sgr |= POST_CELL_SGR_CONCEAL;
          break;
        case 9:
          cursor->sgr |= POST_CELL_SGR_STRIKE;
          break;
        case 21:
          cursor->sgr &= ~POST_CELL_SGR_UNDERLINE;
          cursor->sgr |= POST_CELL_SGR_DBL_UNDERLINE;
          break;
        case 22:
          cursor->sgr &= ~POST_CELL_SGR_BOLD;
          cursor->sgr &= ~POST_CELL_SGR_FAINT;
          break;
        case 23:
          cursor->sgr &= ~POST_CELL_SGR_ITALIC;
          break;
        case 24:
          cursor->sgr &= ~POST_CELL_SGR_UNDERLINE;
          cursor->sgr &= ~POST_CELL_SGR_DBL_UNDERLINE;
          break;
        case 25:
          cursor->sgr &= ~POST_CELL_SGR_SLOW_BLINK;
          cursor->sgr &= ~POST_CELL_SGR_RAPID_BLINK;
          break;
        case 27:
          cursor->sgr &= ~POST_CELL_SGR_INVERT;
          break;
        case 28:
          cursor->sgr &= ~POST_CELL_SGR_CONCEAL;
          break;
        case 29:
          cursor->sgr &= ~POST_CELL_SGR_STRIKE;
          break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
          cursor->fg = sgrColors[current->n - 30];
          break;
        case 39:
          cursor->fg = appState->config.fg;
          break;
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
          cursor->bg = sgrColors[current->n - 40];
          break;
        case 49:
          cursor->bg = appState->config.bg;
          break;
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
          cursor->fg = sgrColors[current->n - 90 + 8];
          break;
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
          cursor->bg = sgrColors[current->n - 100 + 8];
          break;
        default:
          printf("WARNING: unkown CSI value: '%u'\n", current->n);
          break;
      }
    }
  }
}

void
PostParseCSI(PostAppState* appState, PostCursor* cursor, char ch)
{
  if (ch >= '0' && ch <= '9') {
    PostAttribute* currentAttrib = appState->parser.currentAttrib;
    PostAttribute* head          = appState->parser.attribs;

    if (currentAttrib == NULL) {
      PostAttribute* attrib = malloc(sizeof(PostAttribute));

      if (attrib == NULL) {
        printf("WARNING: out of memory; ignoring CSI attribute\n");
        return;
      }

      memset(attrib, 0, sizeof(PostAttribute));

      if (head == NULL) {
        attrib->prev = attrib;
        attrib->next = attrib;
      } else {
        attrib->prev     = head->prev;
        attrib->next     = head;
        head->prev->next = attrib;
        head->prev       = attrib;
      }

      currentAttrib                  = attrib;
      appState->parser.currentAttrib = attrib;
      head                           = attrib;
      appState->parser.attribs       = attrib;
    }

    head->n *= 10;
    head->n += ch - '0';
    return;
  }

  if (ch == ';') {
    appState->parser.currentAttrib = NULL;
    return;
  }

  puint32 gwidth  = appState->grid.width;
  puint32 gheight = appState->grid.height;

  PostAttribute* attribs   = appState->parser.attribs;
  pbool          isPrivate = appState->parser.isPrivate;

  if (ch == 'A') {
    if (attribs == NULL && cursor->y)
      --cursor->y;

    PostAttributesIterate(attribs)
    {
      if (!current->n)
        current->n = 1;
      if (current->n >= cursor->y)
        cursor->y = 0;
      else
        cursor->y -= current->n;
    }

    goto EndCSI;
  }

  if (ch == 'B') {
    if (attribs == NULL) {
      if (++cursor->y >= gheight)
        cursor->y = gheight - 1;
    }

    PostAttributesIterate(attribs)
    {
      if (!current->n)
        current->n = 1;
      cursor->y += current->n;
      if (cursor->y >= gheight)
        cursor->y = gheight - 1;
    }

    goto EndCSI;
  }

  if (ch == 'C') {
    if (attribs == NULL) {
      if (++cursor->x >= gwidth)
        cursor->x = gwidth - 1;
    }

    PostAttributesIterate(attribs)
    {
      if (!current->n)
        current->n = 1;
      cursor->x += current->n;
      if (cursor->x >= gwidth)
        cursor->x = gwidth - 1;
    }

    goto EndCSI;
  }

  if (ch == 'd') {
    if (attribs == NULL)
      cursor->y = 0;

    PostAttributesIterate(attribs)
    {
      if (current->n)
        cursor->y = current->n - 1;
      else
        cursor->y = 0;

      if (cursor->y >= gheight)
        cursor->y = gheight - 1;
    }

    goto EndCSI;
  }

  if (ch == 'D') {
    if (attribs == NULL && cursor->x)
      --cursor->x;

    PostAttributesIterate(attribs)
    {
      if (!current->n)
        current->n = 1;
      if (current->n >= cursor->x)
        cursor->x = 0;
      else
        cursor->x -= current->n;
    }

    goto EndCSI;
  }

  if (ch == 'E') {
    if (attribs == NULL) {
      cursor->x = 0;
      if (++cursor->y >= gheight)
        cursor->y = gheight - 1;
    }

    PostAttributesIterate(attribs)
    {
      if (!current->n)
        current->n = 1;
      cursor->x = 0;
      cursor->y += current->n;
      if (cursor->y >= gheight)
        cursor->y = gheight - 1;
    }

    goto EndCSI;
  }

  if (ch == 'F') {
    if (attribs == NULL) {
      cursor->x = 0;
      if (cursor->y)
        --cursor->y;
    }

    PostAttributesIterate(attribs)
    {
      if (!current->n)
        current->n = 1;
      cursor->x = 0;
      if (current->n >= cursor->y)
        cursor->y = 0;
      else
        cursor->y -= current->n;
    }

    goto EndCSI;
  }

  if (ch == 'G') {
    if (attribs == NULL)
      cursor->x = 0;

    PostAttributesIterate(attribs)
    {
      if (current->n)
        cursor->x = current->n - 1;
      else
        cursor->x = 0;

      if (cursor->x >= gwidth)
        cursor->x = gwidth - 1;
    }

    goto EndCSI;
  }

  if (ch == 'h' && isPrivate) {
    PostParseDECSET(appState);
    goto EndCSI;
  }

  if (ch == 'H') {
    puint32 x = gwidth, y = gheight;

    PostAttributesIterate(attribs)
    {
      if (x == gwidth) {
        x = current->n;
        if (!x)
          x = 1;
        else if (x >= gwidth)
          x = gwidth - 1;
      } else if (y == gheight) {
        y = current->n;
        if (!y)
          y = 1;
        else if (y >= gheight)
          y = gheight - 1;
      }
    }

    if (x == gwidth)
      x = 1;

    if (y == gheight)
      y = 1;

    cursor->x = x - 1;
    cursor->y = y - 1;

    goto EndCSI;
  }

  if (ch == 'J') {
    if (attribs == NULL) {
      /* clear from cursor to end of screen */
      for (puint32 x = cursor->x; x < gwidth; ++x)
        PostGetCell(x, cursor->y).charCode = 0;
      for (puint32 y = cursor->y + 1; y < gheight; ++y)
        for (puint32 x = 0; x < gwidth; ++x)
          PostGetCell(x, y).charCode = 0;
    }

    PostAttributesIterate(attribs)
    {
      if (attribs->n == 0) {
        /* clear from cursor to end of screen */
        for (puint32 x = cursor->x; x < gwidth; ++x)
          PostGetCell(x, cursor->y).charCode = 0;
        for (puint32 y = cursor->y + 1; y < gheight; ++y)
          for (puint32 x = 0; x < gwidth; ++x)
            PostGetCell(x, y).charCode = 0;
      } else if (attribs->n == 1) {
        for (puint32 y = 0; y < cursor->y; ++y)
          for (puint32 x = 0; x < gwidth; ++x)
            PostGetCell(x, y).charCode = 0;

        for (puint32 x = 0; x <= cursor->x; ++x)
          PostGetCell(x, cursor->y).charCode = 0;
      } else if (attribs->n == 2 || attribs->n == 3) {
        // for now, we have no scrollback
        for (puint32 y = 0; y < gheight; ++y)
          for (puint32 x = 0; x < gwidth; ++x)
            PostGetCell(x, y).charCode = 0;
      } else
        printf("WARNING: Invalid Erase in Display Value: '%u'\n", attribs->n);
    }

    goto EndCSI;
  }

  if (ch == 'K') {
    PostParseEL(appState, cursor);
    goto EndCSI;
  }

  if (ch == 'l' && isPrivate) {
    PostAttributesIterate(attribs)
    {
      switch (current->n) {
        case 2004:
          appState->config.bracketed_paste_mode = 0;
          break;
        default:
          printf("WARNING: Unknown DECRST Value: '%u'\n", current->n);
          break;
      }
    }

    goto EndCSI;
  }

  if (ch == 'm') {
    PostParseSGR(appState, cursor);
    goto EndCSI;
  }

  if (ch == 'P') {
    if (attribs == NULL) {
      for (puint32 x = cursor->x + 1; x < gwidth; ++x)
        PostGetCell(x - 1, cursor->y) = PostGetCell(x, cursor->y);
      PostGetCell(gwidth - 1, cursor->y) = (PostCell) { 0 };
    }

    puint32 diff = gwidth - cursor->x;
    PostAttributesIterate(attribs)
    {
      if (!current->n)
        continue;
      else if (current->n > diff)
        current->n = diff;
      for (puint32 x = cursor->x; x < cursor->x + (diff - current->n); ++x)
        PostGetCell(x, cursor->y) = PostGetCell(x + current->n, cursor->y);
      for (puint32 x = gwidth - current->n; x < gwidth; ++x)
        PostGetCell(x, cursor->y) = (PostCell) { 0 };
    }

    goto EndCSI;
  }

  printf("WARNING: unknown CSI: 'ESC [ %s%c'",
         appState->parser.isPrivate ? "? " : "",
         ch);

  if (appState->parser.attribs != NULL) {
    printf(" ARGS=[");
    PostAttributesIterate(appState->parser.attribs)
    {
      printf("%u", current->n);
      if (current->prev != NULL)
        printf(", ");
    }
    printf("]\n");
  } else
    printf("ARGS: <null>\n");

EndCSI:
  appState->parser.state         = POST_PARSER_STATE_NORMAL;
  appState->parser.currentAttrib = NULL;
  appState->parser.attribs       = NULL;
}
