#ifndef POST_FONT_H
#define POST_FONT_H 1

#include "error.h"
#include "types.h"

typedef struct PostFont
{
  char*   path;
  int     numGlyphs;
  puint32 maxAdvance, height;
  pint32  ascender, descender;
  void*   data;
} PostFont;

typedef struct
{
  puint32 width;
  puint32 height;
  puint32 horizontalAdvance;
} PostGlyphMetrics;

int
PostFontSystemInit(void);

PostError
PostFontCreate(PostFont* font);

PostError
PostFontSetSize(PostFont* font, puint32 height);

PostError
PostFontGetGlyphMetrics(PostFont*         font,
                        puint32           charCode,
                        PostGlyphMetrics* glyphMetrics);

PostError
PostFontLoadGlyph(PostFont* font,
                  puint32   charCode,
                  puint32   width,
                  puint32   height,
                  puint32   pitch,
                  puint8*   bitmap);

void
PostFontDestroy(PostFont* font);

void
PostFontSystemFini(void);

#endif
