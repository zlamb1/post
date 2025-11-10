#ifndef POST_CONFIG_H
#define POST_CONFIG_H 1

#include "post/color.h"
#include "post/types.h"

typedef struct
{
  PostColor fg;
  PostColor bg;
  puint8    tabWidth;
  pbool     bracketedPasteMode;
} PostConfig;

void
PostLoadConfig(PostConfig* config);

#endif
