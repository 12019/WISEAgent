// launcher.h -- Launch services from a privileged process

#ifndef LAUNCHER_H__
#define LAUNCHER_H__

#include "session.h"

struct LaunchRequest {
  int  service;
  int  width, height;
};

int  supportsPAM(void);
int  launchChild(int service, struct Session *session);
void setWindowSize(int pty, int width, int height);
int  forkLauncher(void);
void terminateLauncher(void);
void closeAllFds(int *exceptFd, int num);

#endif
