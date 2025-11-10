#ifndef POST_COMPILER_H
#define POST_COMPILER_H 1

#ifdef __GNUC__

#define UNUSED __attribute__((unused))

#else

#define UNUSED

#endif

#endif
