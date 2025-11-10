#ifndef POST_COLOR_H
#define POST_COLOR_H 1

#include "types.h"

typedef struct
{
  puint8 r, g, b, a;
} PostColor;

#define PostColorRGB(R, G, B) { .r = (R), .g = (G), .b = (B), .a = 255 }

#define PostColorRGBA(R, G, B, A) { .r = (R), .g = (G), .b = (B), .a = (A) }

#define PostColorXXX(X) PostColorRGB((X), (X), (X))

#define POST_COLOR_BLACK (PostColor) PostColorXXX(0)
#define POST_COLOR_WHITE (PostColor) PostColorXXX(255)

#endif
