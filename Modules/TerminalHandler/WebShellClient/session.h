// session.h -- Session management for HTTP/HTTPS connections

#ifndef SESSION_H__
#define SESSION_H__

#include <errno.h>

#define NOINTR(x) ({ int i__; while ((i__ = (x)) < 0 && errno == EINTR); i__;})

#define AJAX_TIMEOUT      45
#define DEF_SES_WIDTH     132
#define DEF_SES_HEIGHT    43

struct Session {
  const char       *sessionKey;
  int              done;
  int              pty;
  int              width;
  int              height;
  char             *buffered;
  int              len;
  int              pid;
};

void addToGraveyard(struct Session *session);
void checkGraveyard(void);
void initSession(struct Session *session, const char *sessionKey);
struct Session *newSession(const char *sessionKey);
void destroySession(struct Session *session);
void deleteSession(struct Session *session);
void abandonSession(struct Session *session);
char *newSessionKey(void);
void finishSession(struct Session *session);
void finishAllSessions(void);
struct Session *findSession(int *isNew, const char *sessionKey);
struct Session *findSessionEx(const char *sessionKey);
void iterateOverSessions(int (*fnc)(void *, const char *, char **), void *arg);
int  numSessions(void);

#endif /* SESSION_H__ */
