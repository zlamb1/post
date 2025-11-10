#ifndef POST_PROC_H
#define POST_PROC_H 1

#include <stdio.h>

#include "post.h"

PostError
PostChildProcessSpawn(char*         executable,
                      FILE**        master,
                      PostProcess** childProcess);

PostError
PostChildProcessPoll(PostAppState* appState);

PostError
PostChildProcessSend(PostAppState* appState, const char* buf, pusize size);

void
PostProcessDestroy(PostProcess* proc);

#endif
