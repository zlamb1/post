#ifndef POST_STRING_H
#define POST_STRING_H 1

#include <string.h>

#include "post/error.h"
#include "post/types.h"

typedef struct
{
  pusize size, cap;
  char*  buf;
} PostString;

PostError
PostStringAppendChar(PostString* str, char c);

void
PostStringRelease(PostString* str);

char*
PostCStringDuplicate(const char* s);

#endif
