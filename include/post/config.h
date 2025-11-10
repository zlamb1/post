#ifndef POST_CONFIG_H
#define POST_CONFIG_H 1

#include "post/color.h"
#include "post/types.h"

typedef struct
{
  PostColor fg;
  PostColor bg;
  pbool     bracketed_paste_mode;
} PostConfig;

void
PostLoadConfig(PostConfig* config);

#endif
