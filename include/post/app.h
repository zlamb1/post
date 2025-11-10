#include "post/error.h"
#ifndef POST_APP_H
#define POST_APP_H 1

#include <stdio.h>

#include <SDL3/SDL.h>

#include "color.h"
#include "config.h"
#include "parser.h"
#include "renderer.h"

#define POST_CELL_SGR_BOLD          (1 << 0)
#define POST_CELL_SGR_FAINT         (1 << 1)
#define POST_CELL_SGR_ITALIC        (1 << 2)
#define POST_CELL_SGR_UNDERLINE     (1 << 3)
#define POST_CELL_SGR_SLOW_BLINK    (1 << 4)
#define POST_CELL_SGR_RAPID_BLINK   (1 << 5)
#define POST_CELL_SGR_INVERT        (1 << 6)
#define POST_CELL_SGR_CONCEAL       (1 << 7)
#define POST_CELL_SGR_STRIKE        (1 << 8)
#define POST_CELL_SGR_DBL_UNDERLINE (1 << 9)

typedef struct
{
  puint32   charCode;
  PostColor fg, bg;
  puint16   sgr;
} PostCell;

typedef struct
{
  pusize    byteSize;
  puint32   width, height;
  PostCell* cells;
} PostCellGrid;

typedef struct PostCursor
{
  puint8    visible;
  puint64   time;
  puint32   x, y;
  PostColor fg, bg;
  puint16   sgr;
} PostCursor;

typedef struct PostProcess PostProcess;

typedef struct PostAppState
{
  PostConfig    config;
  PostParser    parser;
  PostCursor    cursor;
  PostCellGrid  grid;
  PostRenderer* renderer;
  FILE*         master;
  PostProcess*  childProcess;
  void (*DestroyApp)(struct PostAppState* appState);
} PostAppState;

void
PostAppWriteASCIIString(PostAppState* appState, const char* str);

PostError
PostAppSizeGrid(PostAppState* appState);

static inline PostError
PostAppSetTitle(PostAppState* appState, const char* title)
{
  if (appState->renderer == NULL || appState->renderer->SetWindowTitle == NULL)
    return POST_ERR_UNSUPPORTED;
  return appState->renderer->SetWindowTitle(appState, title);
}

static inline void
PostAppDestroy(PostAppState* appState)
{
  if (appState == NULL)
    return;
  appState->DestroyApp(appState);
}

#endif
