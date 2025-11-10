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

#include <stddef.h>
#include <stdlib.h>

#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "post/font.h"
#include "post/string.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

static int        isFCInit  = 0;
static FT_Library ftLibrary = NULL;

int
PostFontSystemInit(void)
{
  FT_Error error;

  if (FcInit() != FcTrue)
    return 1;

  isFCInit = 1;

  error = FT_Init_FreeType(&ftLibrary);
  if (error) {
    ftLibrary = NULL;
    return 1;
  }

  return 0;
}

PostError
PostFontCreate(PostFont* font)
{
  PostError error = POST_ERR_OUT_OF_MEMORY;

  FcPattern*   fcPattern   = NULL;
  FcObjectSet* fcObjectSet = NULL;
  FcChar8*     fontPath    = NULL;
  FcPattern*   fcMatch     = NULL;
  FcResult     result;

  FT_Face face = NULL;

  if (!isFCInit)
    return POST_ERR_NEED_INIT;

  fcPattern   = FcPatternCreate();
  fcObjectSet = FcObjectSetBuild(FC_FAMILY, FC_FILE, (char*) 0);

  if (fcPattern == NULL || fcObjectSet == NULL)
    goto fail;

  if (FcPatternAddString(
        fcPattern, FC_FONTFORMAT, (const FcChar8*) "TrueType") == FcFalse)
    goto fail;

  if (FcPatternAddInteger(fcPattern, FC_SPACING, FC_MONO) == FcFalse)
    goto fail;

  if (FcConfigSubstitute(NULL, fcPattern, FcMatchPattern) == FcFalse)
    goto fail;

  FcDefaultSubstitute(fcPattern);

  fcMatch = FcFontMatch(NULL, fcPattern, &result);

  if (result == FcResultOutOfMemory)
    goto fail;

  if (result == FcResultNoMatch ||
      FcPatternGetString(fcMatch, FC_FILE, 0, &fontPath) != FcResultMatch) {
    error = POST_ERR_FONT_NOT_FOUND;
    goto fail;
  }

  font->path = PostCStringDuplicate((const char*) fontPath);

  // TODO: try and load another font?
  if (FT_New_Face(ftLibrary, font->path, 0, &face)) {
    error = POST_ERR_FONT_NOT_FOUND;
    goto fail;
  }

  font->numGlyphs  = face->num_glyphs;
  font->maxAdvance = 0;
  font->height     = 0;
  font->ascender   = 0;
  font->data       = face;

  FcPatternDestroy(fcMatch);
  FcObjectSetDestroy(fcObjectSet);
  FcPatternDestroy(fcPattern);

  return POST_ERR_NONE;

fail:
  if (fcMatch != NULL)
    FcPatternDestroy(fcMatch);

  if (fcObjectSet != NULL)
    FcObjectSetDestroy(fcObjectSet);

  if (fcPattern != NULL)
    FcPatternDestroy(fcPattern);

  return error;
}

PostError
PostFontSetSize(PostFont* font, puint32 height)
{
  FT_Face face = font->data;
  if (FT_Set_Pixel_Sizes(face, 0, height))
    return POST_ERR_NOT_SCALABLE;
  font->maxAdvance = face->size->metrics.max_advance >> 6;
  font->height     = face->size->metrics.height >> 6;
  font->ascender   = face->size->metrics.ascender >> 6;
  return POST_ERR_NONE;
}

PostError
PostFontGetGlyphMetrics(PostFont*         font,
                        puint32           charCode,
                        PostGlyphMetrics* glyphMetrics)
{
  FT_Face          face = font->data;
  FT_Glyph_Metrics metrics;

  if (charCode > 0x10FFFF)
    return POST_ERR_BAD_ARG;

  FT_UInt charIndex = FT_Get_Char_Index(face, charCode);

  if (FT_Load_Glyph(face, charIndex, FT_LOAD_DEFAULT))
    return POST_ERR_RENDER_GLYPH;

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
    return POST_ERR_RENDER_GLYPH;

  metrics = face->glyph->metrics;

  glyphMetrics->width             = metrics.width >> 6;
  glyphMetrics->height            = metrics.height >> 6;
  glyphMetrics->horizontalAdvance = metrics.horiAdvance >> 6;

  return POST_ERR_NONE;
}

PostError
PostFontLoadGlyph(PostFont* font,
                  puint32   charCode,
                  puint32   width,
                  puint32   height,
                  puint32   pitch,
                  puint8*   bitmap)
{
  FT_Face      face = font->data;
  FT_GlyphSlot slot;
  FT_Bitmap    _bitmap;

  if (charCode > 0x10FFFF)
    return POST_ERR_BAD_ARG;

  FT_UInt charIndex = FT_Get_Char_Index(face, charCode);

  if (FT_Load_Glyph(face, charIndex, FT_LOAD_DEFAULT))
    return POST_ERR_RENDER_GLYPH;

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
    return POST_ERR_RENDER_GLYPH;

  memset(bitmap, 0, width * height);

  slot    = face->glyph;
  _bitmap = slot->bitmap;

  if (slot->bitmap_left < 0 || height < (puint32) slot->bitmap_top)
    return POST_ERR_NONE;

  puint32 left = slot->bitmap_left;
  puint32 top;

  if (slot->bitmap_top >= 0)
    top = font->ascender - MIN(font->ascender, (puint32) slot->bitmap_top);
  else
    top = font->ascender - slot->bitmap_top;

  puint32 minWidth  = MIN(width, left + _bitmap.width);
  puint32 minHeight = MIN(height, top + _bitmap.rows);

  for (puint32 y = top; y < minHeight; ++y) {
    for (puint32 x = left; x < minWidth; ++x) {
      bitmap[y * pitch + x] =
        _bitmap.buffer[(y - top) * _bitmap.pitch + (x - left)];
    }
  }

  return POST_ERR_NONE;
}

void
PostFontDestroy(PostFont* font)
{
  FT_Done_Face((FT_Face) font->data);
  free(font->path);
}

void
PostFontSystemFini(void)
{
  if (isFCInit) {
    isFCInit = 0;
    FcFini();
  }

  if (ftLibrary != NULL) {
    FT_Done_FreeType(ftLibrary);
    ftLibrary = NULL;
  }
}
