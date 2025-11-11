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

#include <poll.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "post.h"

typedef struct PostProc
{
  pid_t pid;
} PostProc;

extern char** environ;

PostError
PostChildProcessSpawn(char* executable, FILE** master, PostProc** childProcess)
{
  PostProc* _childProcess = malloc(sizeof(PostProc));
  pid_t     pid;
  int       amaster, aslave;

  if (_childProcess == NULL)
    return POST_ERR_OUT_OF_MEMORY;

  if (openpty(&amaster, &aslave, NULL, NULL, NULL)) {
    free(_childProcess);
    return POST_ERR_POSIX;
  }

  fflush(NULL);

  if ((pid = fork()) == 0) {
    close(amaster);

    if (setsid() == -1) {
      close(aslave);
      exit(1);
    }

    ioctl(aslave, TIOCSCTTY, 0);

    dup2(aslave, STDIN_FILENO);
    dup2(aslave, STDOUT_FILENO);
    dup2(aslave, STDERR_FILENO);

    close(aslave);

    char* argv[] = { executable, NULL };
    execve(executable, argv, environ);
  }

  close(aslave);

  if (pid == -1) {
    close(amaster);
    free(_childProcess);
    return POST_ERR_POSIX;
  }

  *master = fdopen(amaster, "r");
  if (*master == NULL) {
    close(amaster);
    free(_childProcess);
    return POST_ERR_POSIX;
  }

  _childProcess->pid = pid;
  *childProcess      = _childProcess;

  return POST_ERR_NONE;
}

PostError
PostChildProcessPoll(PostAppState* appState)
{
  int fd = fileno(appState->master);
  int result;

  char    buf[256];
  ssize_t bytes;

  struct pollfd fds = {
    .fd     = fd,
    .events = POLLIN,
  };

PollChild:
  result = poll(&fds, 1, 0);

  if (result == 1) {
    if (fds.revents & POLLIN) {
      bytes = read(fd, buf, 255);
      if (bytes > 0) {
        buf[bytes] = 0x0;
        PostAppWriteASCIIString(appState, buf);
        if (bytes == 255)
          goto PollChild;
      }
    }

    return POST_ERR_NONE;
  } else if (!result)
    return POST_ERR_NONE;
  else
    return POST_ERR_POSIX;
}

PostError
PostChildProcessSend(PostAppState* appState, const char* buf, pusize size)
{
  int fd = fileno(appState->master);

  while (size) {
    ssize_t written = write(fd, buf, size);

    if (written < 0)
      return POST_ERR_POSIX;

    if (size < (pusize) written)
      size = 0;
    else {
      size -= written;
      buf += written;
    }
  }

  return POST_ERR_NONE;
}

PostError
PostChildProcessSendWindowSize(PostAppState* appState)
{
  int fd = fileno(appState->master);

  struct winsize winsize = {
    .ws_col = appState->grid.width,
    .ws_row = appState->grid.height,
  };

  if (ioctl(fd, TIOCSWINSZ, &winsize) == -1)
    return POST_ERR_POSIX;

  return POST_ERR_NONE;
}

void
PostProcessDestroy(PostProc* proc)
{
  free(proc);
}
