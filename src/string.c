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

#include "post/string.h"

PostError
PostStringAppendChar(PostString* str, char c)
{
  pusize size = str->size;
  pusize cap  = str->cap;

  if (size >= cap) {
    char* buf;

    if (!cap)
      cap = 1;

    while (size >= cap)
      cap <<= 1;

    buf = realloc(str->buf, cap);
    if (buf == NULL)
      return POST_ERR_OUT_OF_MEMORY;

    str->cap = cap;
    str->buf = buf;
  }

  str->buf[str->size++] = c;

  return POST_ERR_NONE;
}

void
PostStringRelease(PostString* str)
{
  str->size = 0;
  str->cap  = 0;
  free(str->buf);
}

char*
PostCStringDuplicate(const char* s)
{
  const char* tmp = s;
  int         len = 0;

  for (; tmp[0]; ++tmp, ++len)
    ;

  if (!len)
    return NULL;

  char* ns = malloc(len + 1);
  if (ns == NULL)
    return NULL;

  for (int i = 0; i < len; ++i)
    ns[i] = s[i];

  ns[len] = '\0';

  return ns;
}
