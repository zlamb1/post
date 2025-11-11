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

#include "post/app.h"
#include "post/color.h"
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

typedef void (*PostCommand1)(PostAppState*, PostCursor*, puint32);
typedef void (*PostCommand2)(PostAppState*, PostCursor*, puint32, puint32);

typedef struct
{
  puint32 defaultValue;
} PostArg;

typedef struct
{
  /**
   * 1 - accept any args (MUL)
   * 2 - accept up to one args
   * 3 - accept up to two args
   */
  puint8 type;

  union
  {
    struct
    {
      PostArg      arg;
      PostCommand1 command;
    } one_arg;
    struct
    {
      PostArg      args[2];
      PostCommand2 command;
    } two_args;
  };
} PostCommand;

#define DefinePostCommandMul(NAME) DefinePostCommand1(NAME)
#define DefinePostCommand1(NAME)                                               \
  void PostCommand##NAME(                                                      \
    UNUSED PostAppState* appState, UNUSED PostCursor* cursor, puint32 arg)
#define DefinePostCommand2(NAME)                                               \
  void PostCommand##NAME(UNUSED PostAppState* appState,                        \
                         UNUSED PostCursor*   cursor,                          \
                         puint32              arg1,                            \
                         puint32              arg2)

#define PostCommandMulStruct(NAME, DEFAULT_VALUE)                              \
  {                                                                            \
    .type = 1, .one_arg = {                                                    \
      .arg     = { .defaultValue = (DEFAULT_VALUE) },                          \
      .command = PostCommand##NAME,                                            \
    }                                                                          \
  }

#define PostCommand1Struct(NAME, DEFAULT_VALUE)                                \
  {                                                                            \
    .type = 2, .one_arg = {                                                    \
      .arg     = { .defaultValue = (DEFAULT_VALUE) },                          \
      .command = PostCommand##NAME,                                            \
    }                                                                          \
  }

#define PostCommand2Struct(NAME, DEFAULT_VALUE1, DEFAULT_VALUE2)               \
  {                                                                            \
    .type = 3, \
    .two_args = {                                                   \
      .args    = { \
          { .defaultValue = (DEFAULT_VALUE1) }, \
          { .defaultValue = (DEFAULT_VALUE2), }, \
        }, \
      .command = PostCommand##NAME, \
    }                                                               \
  }

DefinePostCommand1(ICH)
{
  if (!arg)
    arg = 1;

  cursor->lastColumnFlag = 0;

  if (cursor->x + arg > PostGridWidth())
    arg = PostGridWidth() - cursor->x;

  for (puint32 x = PostGridWidth() - 1; x >= cursor->x + arg; --x)
    PostGetCell(x, cursor->y) = PostGetCell(x - arg, cursor->y);

  for (puint32 x = cursor->x; x < cursor->x + arg; ++x)
    PostGetCell(x, cursor->y).charCode = 0;
}

DefinePostCommand1(CUU)
{
  if (!arg)
    arg = 1;

  cursor->lastColumnFlag = 0;

  if (arg >= cursor->y)
    cursor->y = 0;
  else
    cursor->y -= arg;
}

DefinePostCommand1(CUD)
{
  if (!arg)
    arg = 1;

  cursor->lastColumnFlag = 0;

  if ((cursor->y += arg) >= PostGridHeight())
    cursor->y = PostGridHeight() - 1;
}

DefinePostCommand1(CUF)
{
  if (!arg)
    arg = 1;

  cursor->lastColumnFlag = 0;

  if ((cursor->x += arg) >= PostGridWidth())
    cursor->x = PostGridWidth() - 1;
}

DefinePostCommand1(CUB)
{
  if (!arg)
    arg = 1;

  cursor->lastColumnFlag = 0;

  if (arg >= cursor->x)
    cursor->x = 0;
  else
    cursor->x -= arg;
}

DefinePostCommand1(CNL)
{
  if (!arg)
    arg = 1;

  cursor->x = 0;
  if ((cursor->y += arg) >= PostGridHeight())
    cursor->y = PostGridHeight() - 1;
}

DefinePostCommand1(CPL)
{
  if (!arg)
    arg = 1;

  cursor->x = 0;
  if (cursor->y <= arg)
    cursor->y = 0;
  else
    cursor->y -= arg;
}

DefinePostCommand1(CHA)
{
  if (!arg)
    arg = 1;

  if (arg >= PostGridWidth())
    arg = PostGridWidth() - 1;

  cursor->x = arg;
}

DefinePostCommand2(CUP)
{
  if (!arg1)
    arg1 = 1;
  else if (arg1 > PostGridHeight())
    arg1 = PostGridHeight();

  if (!arg2)
    arg2 = 1;
  else if (arg2 > PostGridWidth())
    arg2 = PostGridWidth();

  cursor->lastColumnFlag = 0;
  cursor->x              = arg2 - 1;
  cursor->y              = arg1 - 1;
}

// FIXME!: think about user defined tab stops like with HTS
DefinePostCommand1(CHT)
{
  puint32 tabWidth = appState->config.tabWidth;
  if (!arg)
    arg = 1;
  // truncate to last tab stop
  cursor->x = cursor->x / tabWidth * tabWidth;
  cursor->x += arg * tabWidth;
  if (cursor->x >= PostGridWidth())
    cursor->x = PostGridWidth() - 1;
}

DefinePostCommand1(ED)
{
  PostCell defaultCell = {
    .fg = appState->config.fg,
    .bg = appState->config.bg,
  };

  cursor->lastColumnFlag = 0;

  if (arg == 0) {
    for (puint32 x = cursor->x; x < PostGridWidth(); ++x)
      PostGetCell(x, cursor->y) = defaultCell;

    for (puint32 y = cursor->y + 1; y < PostGridHeight(); ++y)
      for (puint32 x = 0; x < PostGridWidth(); ++x)
        PostGetCell(x, y) = defaultCell;
  } else if (arg == 1) {
    for (puint32 y = 0; y < cursor->y; ++y)
      for (puint32 x = 0; x < PostGridWidth(); ++x)
        PostGetCell(x, y) = defaultCell;

    for (puint32 x = 0; x < cursor->x; ++x)
      PostGetCell(x, cursor->y) = defaultCell;
  } else if (arg == 2 || arg == 3) {
    // FIXME: also clear scrollback buffer on attrib == 3 once we implement
    // scrollback
    for (puint32 y = 0; y < PostGridHeight(); ++y)
      for (puint32 x = 0; x < PostGridWidth(); ++x)
        PostGetCell(x, y) = defaultCell;
  } else
    PostAppLogWarning(appState, "Invalid Erase in Display Argument: %u", arg);
}

DefinePostCommand1(EL)
{
  puint32  start, end;
  PostCell defaultCell = {
    .fg = appState->config.fg,
    .bg = appState->config.bg,
  };

  cursor->lastColumnFlag = 0;

  if (arg == 0) {
    start = cursor->x;
    end   = PostGridWidth();
  } else if (arg == 1) {
    start = 0;
    end   = cursor->x;
  } else if (arg == 2) {
    start = 0;
    end   = PostGridWidth();
  } else {
    PostAppLogWarning(appState, "Invalid Erase in Line Argument: %u", arg);
    return;
  }

  for (; start < end; ++start)
    PostGetCell(start, cursor->y) = defaultCell;
}

DefinePostCommand1(DECSET)
{
  switch (arg) {
    case 2004:
      appState->config.bracketedPasteMode = 1;
      break;
    default:
      PostAppLogWarning(appState, "Invalid DECSET Argument: %u", arg);
      break;
  }
}

DefinePostCommand1(DECRST)
{
  switch (arg) {
    case 2004:
      appState->config.bracketedPasteMode = 0;
      break;
    default:
      PostAppLogWarning(appState, "Invalid DECRST Argument: %u", arg);
      break;
  }
}

DefinePostCommandMul(SGR)
{
  switch (arg) {
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
      cursor->fg = sgrColors[arg - 30];
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
      cursor->bg = sgrColors[arg - 40];
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
      cursor->fg = sgrColors[arg - 90 + 8];
      break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
      cursor->bg = sgrColors[arg - 100 + 8];
      break;
    default:
      PostAppLogWarning(appState, "Invalid SGR Argument: %u", arg);
      break;
  }
}

static PostCommand commands[128] = {
  [POST_UNICODE_AT_SIGN] = PostCommand1Struct(ICH, 1),
  [POST_UNICODE_A]       = PostCommand1Struct(CUU, 1),
  [POST_UNICODE_B]       = PostCommand1Struct(CUD, 1),
  [POST_UNICODE_C]       = PostCommand1Struct(CUF, 1),
  [POST_UNICODE_D]       = PostCommand1Struct(CUB, 1),
  [POST_UNICODE_E]       = PostCommand1Struct(CNL, 1),
  [POST_UNICODE_F]       = PostCommand1Struct(CPL, 1),
  [POST_UNICODE_G]       = PostCommand1Struct(CHA, 1),
  [POST_UNICODE_H]       = PostCommand2Struct(CUP, 1, 1),
  [POST_UNICODE_I]       = PostCommand1Struct(CHT, 1),
  [POST_UNICODE_J]       = PostCommand1Struct(ED, 0),
  [POST_UNICODE_K]       = PostCommand1Struct(EL, 0),
  [POST_UNICODE_m]       = PostCommandMulStruct(SGR, 0),
  // TODO: CUP
};

static PostCommand privateCommands[128] = {
  [POST_UNICODE_h] = PostCommand1Struct(DECSET, 0),
  [POST_UNICODE_l] = PostCommand1Struct(DECRST, 0),
};

void
PostParseCSI(PostAppState* appState, PostCursor* cursor, char ch)
{
  if (appState->parser.attribs == NULL && ch == POST_UNICODE_QUESTION_MARK) {
    appState->parser.isPrivate = 1;
    return;
  }

  if (ch >= POST_UNICODE_0 && ch <= POST_UNICODE_9) {
    PostAttribute* currentAttrib = appState->parser.currentAttrib;
    PostAttribute* head          = appState->parser.attribs;

    if (currentAttrib == NULL) {
      PostAttribute* attrib = malloc(sizeof(PostAttribute));

      if (attrib == NULL) {
        PostAppLogWarning(appState, "Ignoring CSI Attribute: Out Of Memory");
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

  PostAttribute* attribs   = appState->parser.attribs;
  pbool          isPrivate = appState->parser.isPrivate;

#if 1
  PostAppLogInfo(appState, "CSI: %c", ch);
#endif

  if (ch >= 0) {
    PostCommand command =
      isPrivate ? privateCommands[(unsigned) ch] : commands[(unsigned) ch];

    if (command.type > 0) {
      if (command.type == 1) {
        PostAttributesIterate(attribs) command.one_arg.command(
          appState,
          cursor,
          current->isEmpty ? command.one_arg.arg.defaultValue : current->n);
      } else if (command.type == 2) {
        if (attribs == NULL) {
          command.one_arg.command(
            appState, cursor, command.one_arg.arg.defaultValue);
          goto EndCSI;
        }

        command.one_arg.command(appState,
                                cursor,
                                attribs->prev->isEmpty
                                  ? command.one_arg.arg.defaultValue
                                  : attribs->prev->n);

        // NOTE: free attribs
        PostAttributesIterate(attribs);
      } else if (command.type == 3) {
        puint32 c = 0, arg1, arg2;

        if (attribs == NULL) {
          command.two_args.command(appState,
                                   cursor,
                                   command.two_args.args[0].defaultValue,
                                   command.two_args.args[1].defaultValue);
          goto EndCSI;
        }

        PostAttributesIterate(attribs)
        {
          if (c++ == 0) {
            arg1 = current->isEmpty ? command.two_args.args[0].defaultValue
                                    : current->n;
          } else {
            arg2 = current->isEmpty ? command.two_args.args[1].defaultValue
                                    : current->n;
            c    = 0;
            command.two_args.command(appState, cursor, arg1, arg2);
          }
        }

        if (c)
          command.two_args.command(
            appState, cursor, arg1, command.two_args.args[1].defaultValue);
      } else {
        PostAppLogWarning(appState, "Internal Error: Invalid Command Type");
        PostAttributesIterate(attribs);
      }

      goto EndCSI;
    }
  }

  PostAppLogWarning(
    appState, "Unknown Command Sequence Introducer: ESC[%c", (unsigned) ch, ch);

EndCSI:
  appState->parser.state         = POST_PARSER_STATE_NORMAL;
  appState->parser.currentAttrib = NULL;
  appState->parser.attribs       = NULL;
}
