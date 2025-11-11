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
#include "post/compiler.h"
#include "post/parser.h"
#include "post/unicode.h"

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

#define PostGetCell(X, Y) appState->grid.cells[(Y) * appState->grid.width + (X)]

#define PostGridWidth()  appState->grid.width
#define PostGridHeight() appState->grid.height

typedef void (*PostIteratorCommand)(PostAppState* appState,
                                    PostCursor*   cursor,
                                    puint32       attrib,
                                    pbool         isEmpty);

typedef void (*PostAggregateCommand)(PostAppState*  appState,
                                     PostCursor*    cursor,
                                     PostAttribute* attribs);

#define CreateIteratorCommand(NAME)                                            \
  static void PostCommand##NAME(UNUSED PostAppState* appState,                 \
                                UNUSED PostCursor*   cursor,                   \
                                puint32              attrib,                   \
                                pbool                isEmpty)

#define CreateAggregateCommand(NAME)                                           \
  static void PostCommand##NAME(UNUSED PostAppState* appState,                 \
                                UNUSED PostCursor*   cursor,                   \
                                PostAttribute*       attribs)

CreateIteratorCommand(ICH)
{
  if (isEmpty)
    attrib = 1;

  if (!attrib)
    return;

  if (cursor->x + attrib > PostGridWidth())
    attrib = PostGridWidth() - cursor->x;

  for (puint32 x = PostGridWidth() - 1; x >= cursor->x + attrib; --x)
    PostGetCell(x, cursor->y) = PostGetCell(x - attrib, cursor->y);

  for (puint32 x = cursor->x; x < cursor->x + attrib; ++x)
    PostGetCell(x, cursor->y).charCode = 0;
}

CreateIteratorCommand(CUU)
{
  if (isEmpty)
    attrib = 1;

  if (attrib >= cursor->y)
    cursor->y = 0;
  else
    cursor->y -= attrib;
}

CreateIteratorCommand(CUD)
{
  if (isEmpty)
    attrib = 1;

  if ((cursor->y += attrib) >= PostGridHeight())
    cursor->y = PostGridHeight() - 1;
}

CreateIteratorCommand(CUF)
{
  if (isEmpty)
    attrib = 1;

  if ((cursor->x += attrib) >= PostGridWidth())
    cursor->x = PostGridWidth() - 1;
}

CreateIteratorCommand(CUB)
{
  if (isEmpty)
    attrib = 1;

  if (attrib >= cursor->x)
    cursor->x = 0;
  else
    cursor->x -= attrib;
}

CreateIteratorCommand(CNL)
{
  if (isEmpty)
    attrib = 1;

  cursor->x = 0;
  if ((cursor->y += attrib) >= PostGridHeight())
    cursor->y = PostGridHeight() - 1;
}

CreateIteratorCommand(CPL)
{
  if (isEmpty)
    attrib = 1;

  cursor->x = 0;
  if (cursor->y <= attrib)
    cursor->y = 0;
  else
    cursor->y -= attrib;
}

CreateIteratorCommand(CHA)
{
  if (isEmpty)
    attrib = 1;

  if (attrib >= PostGridWidth())
    attrib = PostGridWidth() - 1;

  cursor->x = attrib;
}

CreateAggregateCommand(CUP)
{
  PostAttribute* current;
  puint32        c = 0, x, y;

  if (attribs == NULL) {
    cursor->x = 0;
    cursor->y = 0;
    return;
  }

  current = attribs->prev;

  do {
    if (c) {
      if (current->isEmpty)
        x = 0;
      else {
        x = current->n;
        if (x)
          --x;
        if (x >= PostGridWidth())
          x = PostGridWidth() - 1;
      }

      cursor->x = x;
      cursor->y = y;

      c = 0;
    } else {
      if (current->isEmpty)
        y = 0;
      else {
        y = current->n;
        if (y)
          --y;
        if (y >= PostGridHeight())
          y = PostGridHeight() - 1;
      }
      ++c;
    }

    current = current->prev;
  } while (current != attribs->prev);

  if (c) {
    cursor->x = 0;
    cursor->y = y;
  }
}

// FIXME!: think about user defined tab stops like with HTS
CreateIteratorCommand(CHT)
{
  puint32 tabWidth = appState->config.tabWidth;
  if (isEmpty)
    attrib = 1;
  // truncate to last tab stop
  cursor->x = cursor->x / tabWidth * tabWidth;
  cursor->x += attrib * tabWidth;
  if (cursor->x >= PostGridWidth())
    cursor->x = PostGridWidth() - 1;
}

CreateIteratorCommand(ED)
{
  PostCell defaultCell = {
    .fg = appState->config.fg,
    .bg = appState->config.bg,
  };

  if (isEmpty)
    attrib = 0;

  if (attrib == 0) {
    for (puint32 x = cursor->x; x < PostGridWidth(); ++x)
      PostGetCell(x, cursor->y) = defaultCell;

    for (puint32 y = cursor->y + 1; y < PostGridHeight(); ++y)
      for (puint32 x = 0; x < PostGridWidth(); ++x)
        PostGetCell(x, y) = defaultCell;
  } else if (attrib == 1) {
    for (puint32 y = 0; y < cursor->y; ++y)
      for (puint32 x = 0; x < PostGridWidth(); ++x)
        PostGetCell(x, y) = defaultCell;

    for (puint32 x = 0; x < cursor->x; ++x)
      PostGetCell(x, cursor->y) = defaultCell;
  } else if (attrib == 2 || attrib == 3) {
    // FIXME: also clear scrollback buffer on attrib == 3 once we implement
    // scrollback
    for (puint32 y = 0; y < PostGridHeight(); ++y)
      for (puint32 x = 0; x < PostGridWidth(); ++x)
        PostGetCell(x, y) = defaultCell;
  } else {
    // FIXME: log warning
  }
}

CreateIteratorCommand(EL)
{
  puint32  start, end;
  PostCell defaultCell = {
    .fg = appState->config.fg,
    .bg = appState->config.bg,
  };

  if (isEmpty)
    attrib = 0;

  if (attrib == 0) {
    start = cursor->x;
    end   = PostGridWidth();
  } else if (attrib == 1) {
    start = 0;
    end   = cursor->x;
  } else if (attrib == 2) {
    start = 0;
    end   = PostGridWidth();
  } else {
    // FIXME: log warning
    return;
  }

  for (; start < end; ++start)
    PostGetCell(start, cursor->y) = defaultCell;
}

CreateIteratorCommand(DECSET)
{
  if (isEmpty)
    // FIXME: log warning
    return;

  switch (attrib) {
    case 2004:
      appState->config.bracketedPasteMode = 1;
      break;
    default:
      // FIXME: log warning
      break;
  }
}

CreateIteratorCommand(DECRST)
{
  if (isEmpty)
    // FIXME: log warning
    return;

  switch (attrib) {
    case 2004:
      appState->config.bracketedPasteMode = 0;
      break;
    default:
      // FIXME: log warning
      break;
  }
}

CreateIteratorCommand(SGR)
{
  if (isEmpty)
    attrib = 0;

  switch (attrib) {
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
      cursor->fg = sgrColors[attrib - 30];
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
      cursor->bg = sgrColors[attrib - 40];
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
      cursor->fg = sgrColors[attrib - 90 + 8];
      break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
      cursor->bg = sgrColors[attrib - 100 + 8];
      break;
    default:
      // FIXME: log warning
      break;
  }
}

static PostIteratorCommand iteratorCommands[128] = {
  [POST_UNICODE_AT_SIGN] = PostCommandICH, [POST_UNICODE_A] = PostCommandCUU,
  [POST_UNICODE_B] = PostCommandCUD,       [POST_UNICODE_C] = PostCommandCUF,
  [POST_UNICODE_D] = PostCommandCUB,       [POST_UNICODE_E] = PostCommandCNL,
  [POST_UNICODE_F] = PostCommandCPL,       [POST_UNICODE_G] = PostCommandCHA,
  [POST_UNICODE_I] = PostCommandCHT,       [POST_UNICODE_J] = PostCommandED,
  [POST_UNICODE_K] = PostCommandEL,        [POST_UNICODE_m] = PostCommandSGR,
};

static PostIteratorCommand privateIteratorCommands[128] = {
  [POST_UNICODE_h] = PostCommandDECSET,
  [POST_UNICODE_l] = PostCommandDECRST,
};

static PostAggregateCommand aggregateCommands[128] = {
  [POST_UNICODE_H] = PostCommandCUP,
};

static PostAggregateCommand privateAggregateCommands[128] = { 0 };

void
PostParseCSI(PostAppState* appState, PostCursor* cursor, char ch)
{
  if (ch >= POST_UNICODE_0 && ch <= POST_UNICODE_9) {
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
    head->n += ch - POST_UNICODE_0;
    return;
  }

  if (ch == POST_UNICODE_SEMICOLON) {
    appState->parser.currentAttrib = NULL;
    return;
  }

  puint32 gwidth  = appState->grid.width;
  puint32 gheight = appState->grid.height;

  PostAttribute* attribs   = appState->parser.attribs;
  pbool          isPrivate = appState->parser.isPrivate;

#if 0
  printf("CSI: %c ARGS=", ch);
  if (attribs != NULL) {
    PostAttribute* current = attribs->prev;
    printf("[");
    do {
      printf("%u", current->n);
      current = current->prev;
      if (current != attribs->prev)
        printf(", ");
    } while (current != attribs->prev);
    printf("]");
  } else
    printf("<null>");
  printf("\n");
#endif

  if (ch >= 0) {
    PostIteratorCommand iteratorCommand =
      isPrivate ? privateIteratorCommands[(unsigned) ch]
                : iteratorCommands[(unsigned) ch];
    PostAggregateCommand aggregateCommand;

    if (iteratorCommand != NULL) {
      if (attribs == NULL)
        iteratorCommand(appState, cursor, 0, 1);

      PostAttributesIterate(attribs)
        iteratorCommand(appState, cursor, current->n, current->isEmpty);

      goto EndCSI;
    }

    aggregateCommand = isPrivate ? privateAggregateCommands[(unsigned) ch]
                                 : aggregateCommands[(unsigned) ch];

    if (aggregateCommand != NULL) {
      aggregateCommand(appState, cursor, attribs);
      // NOTE: free attributes
      PostAttributesIterate(attribs);
      goto EndCSI;
    }
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
