#ifndef POST_SDL_LOG_H
#define POST_SDL_LOG_H 1

#define PostLogError(ERR) SDL_LogError(SDL_LOG_PRIORITY_ERROR, (ERR))

#define PostLogErrorA(ERR, ...)                                                \
  SDL_LogError(SDL_LOG_PRIORITY_ERROR, (ERR), __VA_ARGS__)

#endif
