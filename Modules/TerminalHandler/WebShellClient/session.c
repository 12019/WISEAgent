// session.c -- Session management for HTTP/HTTPS connections

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "session.h"
#include "hashmap.h"
#include "check.h"

#ifdef HAVE_UNUSED
#defined ATTR_UNUSED __attribute__((unused))
#defined UNUSED(x)   do { } while (0)
#else
#define ATTR_UNUSED
#define UNUSED(x)    do { (void)(x); } while (0)
#endif

static struct HashMap *sessions;


static struct Graveyard {
  struct Graveyard *next;
  time_t           timeout;
  const char       *sessionKey;
} *graveyard;

void addToGraveyard(struct Session *session) {
  // It is possible for a child process to die, but for the Session to
  // linger around, because the browser has also navigated away and thus
  // nobody ever calls completePendingRequest(). We put these Sessions into
  // the graveyard and reap them after a while.
  struct Graveyard *g;
  check(g       = malloc(sizeof(struct Graveyard)));
  g->next       = graveyard;
  g->timeout    = time(NULL) + AJAX_TIMEOUT;
  g->sessionKey = strdup(session->sessionKey);
  graveyard     = g;
}

static void checkGraveyardInternal(int expireAll) {
  if (!graveyard) {
    return;
  }
  time_t timeout = time(NULL) - (expireAll ? 2*AJAX_TIMEOUT : 0);
  for (struct Graveyard **g = &graveyard, *old = *g;
       old; ) {
    if (old->timeout < timeout) {
      *g         = old->next;
      deleteFromHashMap(sessions, old->sessionKey);
      free((char *)old->sessionKey);
      free(old);
    } else {
      g          = &old->next;
    }
    old          = *g;
  }
}

void checkGraveyard(void) {
  checkGraveyardInternal(0);
}

void initSession(struct Session *session, const char *sessionKey) {
  session->sessionKey     = sessionKey;
  session->done           = 0;
  session->pty            = -1;
  session->width          = 0;
  session->height         = 0;
  session->buffered       = NULL;
  session->len            = 0;
}

struct Session *newSession(const char *sessionKey) {
  struct Session *session;
  check(session = malloc(sizeof(struct Session)));
  initSession(session, sessionKey);
  return session;
}

void destroySession(struct Session *session) {
  if (session) {
    free((char *)session->sessionKey);
    if (session->pty >= 0) {
      NOINTR(close(session->pty));
    }
  }
}

void deleteSession(struct Session *session) {
  destroySession(session);
  free(session);
}

void abandonSession(struct Session *session) {
  deleteFromHashMap(sessions, session->sessionKey);
}

void finishSession(struct Session *session) {
  deleteFromHashMap(sessions, session->sessionKey);
}

void finishAllSessions(void) {
  checkGraveyardInternal(1);
  deleteHashMap(sessions);
}

static void destroySessionHashEntry(void *arg ATTR_UNUSED,
                                    char *key ATTR_UNUSED, char *value) {
  UNUSED(arg);
  UNUSED(key);
  deleteSession((struct Session *)value);
}

char *newSessionKey(void) {
  int fd;
  check((fd = NOINTR(open("/dev/urandom", O_RDONLY))) >= 0);
  unsigned char buf[16];
  check(NOINTR(read(fd, buf, sizeof(buf))) == sizeof(buf));
  NOINTR(close(fd));
  char *sessionKey;
  check(sessionKey   = malloc((8*sizeof(buf) + 5)/6 + 1));
  char *ptr          = sessionKey;
  int count          = 0;
  int bits           = 0;
  for (unsigned i = 0;;) {
    bits             = (bits << 8) | buf[i];
    count           += 8;
  drain:
    while (count >= 6) {
      *ptr++         = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
                       "ghijklmnopqrstuvwxyz0123456789-/"
                       [(bits >> (count -= 6)) & 0x3F];
    }
    if (++i >= sizeof(buf)) {
      if (count && i == sizeof(buf)) {
        bits       <<= 8;
        count       += 8;
        goto drain;
      } else {
        break;
      }
    }
  }
  *ptr               = '\000';
  check(!sessions || !getFromHashMap(sessions, sessionKey));
  return sessionKey;
}

struct Session *findSession(int *isNew, const char *sessionKey){
	*isNew                 = 1;
	if (!sessions) {
		sessions = newHashMap(destroySessionHashEntry, NULL);
	}
	struct Session *session= NULL;
	if (sessionKey && *sessionKey) {
		session = (struct Session *)getFromHashMap(sessions,sessionKey);
	}
	if (session) {
		*isNew = 0;
	}else {
		// First contact. Create session, now.
		char * newKey = NULL;
		if(!sessionKey) newKey = newSessionKey();
		else
		{
			int len = strlen(sessionKey)+1;
			newKey = (char *)malloc(len);
			memset(newKey, 0, len);
			strcpy(newKey, sessionKey);
		}
		session = newSession(newKey );
		addToHashMap(sessions, (const char *)newKey, (const char *)session);
		debug("Creating a new session: %s", newKey);
	}
	return session;
}

struct Session *findSessionEx(const char *sessionKey)
{
	struct Session *session= NULL;
	if (sessions) 
	{
		if (sessionKey && *sessionKey) {
			session = (struct Session *)getFromHashMap(sessions,sessionKey);
		}
	}
	return session;
}

void iterateOverSessions(int (*fnc)(void *, const char *, char **), void *arg){
	if (sessions) {
		iterateOverHashMap(sessions, fnc, arg);
	}
}

int numSessions(void) {
  return getHashmapSize(sessions);
}
