#ifndef POST_PARSER_H
#define POST_PARSER_H 1

#include "post/string.h"
#include "post/types.h"

#define POST_PARSER_STATE_NORMAL       0
#define POST_PARSER_STATE_ESC          1
#define POST_PARSER_STATE_DESIGNATE_G0 2
#define POST_PARSER_STATE_CSI          3
#define POST_PARSER_STATE_OSC          4

typedef struct PostCursor   PostCursor;
typedef struct PostAppState PostAppState;

typedef struct PostAttribute
{
  puint32               n;
  pbool                 isEmpty;
  struct PostAttribute *prev, *next;
} PostAttribute;

typedef struct
{
  puint8 state;
  pbool  isPrivate;
  union
  {
    struct
    {
      pbool      parseText;
      puint8     s;
      PostString t;
    };
    struct
    {
      PostAttribute* currentAttrib;
      PostAttribute* attribs;
    };
  };
} PostParser;

void
PostParseCSI(PostAppState* appState, PostCursor* cursor, char ch);

#endif
