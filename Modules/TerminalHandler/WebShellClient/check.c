//#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <check.h>

static int verbosity = MSG_DEFAULT;

static void debugMsg(int level, const char *fmt, va_list ap) {
	if (level <= verbosity) {
		vfprintf(stderr, fmt, ap);
		fputs("\n", stderr);
	}
}

void fatal(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	debugMsg(MSG_QUIET, fmt, ap);
	va_end(ap);
	_exit(1);
}

char *vStringPrintf(char *buf, const char *fmt, va_list ap) {
	int offset    = buf ? strlen(buf) : 0;
	int len       = 80;
	check(buf     = realloc(buf, offset + len));
	va_list aq;
	va_copy(aq, ap);
	int p         = vsnprintf(buf + offset, len, fmt, aq);
	va_end(aq);
	if (p >= len) {
		check(buf   = realloc(buf, offset + p + 1));
		va_copy(aq, ap);
		check(vsnprintf(buf + offset, p + 1, fmt, aq) == p);
		va_end(aq);
	} else if (p < 0) {
		int inc     = 256;
		do {
			len      += inc;
			check(len < (1 << 20));
			if (inc < (32 << 10)) {
				inc   <<= 1;
			}
			check(buf = realloc(buf, offset + len));
			va_copy(aq, ap);
			p         = vsnprintf(buf + offset, len, fmt, ap);
			va_end(aq);
		} while (p < 0 || p >= len);
	}
	return buf;
}

char *stringPrintf(char *buf, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	char *s = vStringPrintf(buf, fmt, ap);
	va_end(ap);
	return s;
}

void debug(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	debugMsg(MSG_DEBUG, fmt, ap);
	va_end(ap);
}

void error(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	debugMsg(MSG_ERROR, fmt, ap);
	va_end(ap);
}

int logIsDebug(void) {
	return verbosity >= MSG_DEBUG;
}
