#ifndef _CHECK_H_
#define _CHECK_H_

#include <stdarg.h>

#define MSG_QUIET  -1
#define MSG_ERROR   1
#define MSG_DEBUG   4
#define MSG_DEFAULT MSG_ERROR

#define check(x)  do {if (!(x)) fatal("Check failed at "__FILE__":%d in %s(): %s", __LINE__, __func__, #x);} while (0)

#define dcheck(x) do {if (!(x)) (logIsDebug() ? fatal : error)( "Check failed at "__FILE__":%d in %s(): %s", __LINE__, __func__, #x);} while (0)

void fatal(const char *fmt, ...)   __attribute__((format(printf, 1, 2), noreturn));
void error(const char *fmt, ...)   __attribute__((format(printf, 1, 2)));
void debug(const char *fmt, ...)   __attribute__((format(printf, 1, 2)));

char *vStringPrintf(char *buf, const char *fmt, va_list ap);
char *stringPrintf(char *buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

int logIsDebug(void);

#endif
