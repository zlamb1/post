#ifndef POST_RENDERER_H
#define POST_RENDERER_H 1

#include "post/error.h"
#include "post/types.h"

typedef struct PostAppState PostAppState;

typedef struct PostRenderer
{
  puint32 windowWidth, windowHeight;
  puint32 cellWidth, cellHeight;
  PostError (*RenderFrame)(PostAppState* appState);
  PostError (*SetWindowTitle)(PostAppState* appState, const char* title);
} PostRenderer;

void
PostRenderFrame(PostAppState* appState);

#endif
