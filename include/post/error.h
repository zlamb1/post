#ifndef POST_ERROR_H
#define POST_ERROR_H 1

#define POST_ERR_NONE             0
#define POST_ERR_SUBSYS           -1
#define POST_ERR_NEED_INIT        -2
#define POST_ERR_BAD_ARG          -3
#define POST_ERR_OUT_OF_MEMORY    -4
#define POST_ERR_FONT_NOT_FOUND   -5
#define POST_ERR_NOT_SCALABLE     -6
#define POST_ERR_RENDER_GLYPH     -7
#define POST_ERR_CHAR_NOT_FOUND   -8
#define POST_ERR_POSIX            -9
#define POST_ERR_UNSUPPORTED      -10
#define POST_ERR_BAD_FONT_METRICS -11

#define PostTry(EXPR)                                                          \
  do {                                                                         \
    PostError error = (EXPR);                                                  \
    if (error != POST_ERR_NONE)                                                \
      return error;                                                            \
  } while (0)

typedef int PostError;

static inline const char*
PostErrorString(PostError error)
{
  switch (error) {
    case POST_ERR_NONE:
      return "No Error";
    case POST_ERR_SUBSYS:
      return "Subsystem";
    case POST_ERR_NEED_INIT:
      return "System Init Required";
    case POST_ERR_BAD_ARG:
      return "Bad Argument";
    case POST_ERR_OUT_OF_MEMORY:
      return "Out Of Memory";
    case POST_ERR_FONT_NOT_FOUND:
      return "Font Not Found";
    case POST_ERR_NOT_SCALABLE:
      return "Font Not Scalable";
    case POST_ERR_RENDER_GLYPH:
      return "Failed to Render Glyph";
    case POST_ERR_POSIX:
      return "Posix Error";
    case POST_ERR_UNSUPPORTED:
      return "Unsupported Operation";
    case POST_ERR_BAD_FONT_METRICS:
      return "Invalid Font Metrics";
    default:
      return "Unknown Error";
  }
}

#endif
