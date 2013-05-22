#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "xmem.h"

static inline const char *
chr2esc(char c)
{
	switch (c) {
	case '"':
		return "\\\"";
	case '\n':
		return "\\n";
	case '\\':
		return "\\\\";
	default:
		return NULL;
	}
}

static inline char
esc2chr(char c)
{
	switch (c) {
	case 'n':
		return '\n';
	default:
		return c;
	}
}

#if 0
static size_t
compute_enc_buf_size(const char *s)
{
	size_t		 size = 0;
	struct xlat_tab	*tp;

	while (*s != '\0') {
		for (tp = tab; tp->s != NULL; ++tp) {
			if (*s == tp->c) {
				size += strlen(tp->s);
				break;
			}
		}
		if (tp->s == NULL)
			size += 1;
		++s;
	}
	size += 1; /* '\0' */

	return size;
}
#endif

#if 0
char *
descr_decode(char *s)
{
	char	*buf;
	char	*bufp;
	char	*p, *q;
	size_t	 slen;

	slen = strlen(s);
	buf = xmalloc(slen + 1);

	s[slen - 1] = '\0'; // remove the last quote
	for (p = s + 1, q = buf; *p != '\0'; ++p, ++q) {
		if (*p == '\\') {
			++p;
			*q = esc2chr(*p);
		} else {
			*q = *p;
		}
	}

	return buf;
}
#endif

char *
descr_decode(const char *s)
{
	size_t		 slen;
	const char	*sp;
	const char	*send;
	char		*buf = NULL;
	size_t		 bufsize = 0;
	char		*bufp;

	slen = strlen(s);
	send = s + slen - 1;
	/* compute the size of the buffer we need */
	for (sp = s + 1; sp < send; /* empty */) {
		bufsize += 1;
		if (*sp == '\\')
			sp += 2;
		else
			++sp;
	}
	bufsize += 1; /* '\0' */

	buf = xmalloc(bufsize);
	/* decode */
	for (sp = s + 1, bufp = buf; sp < send; ++sp, ++bufp) {
		if (*sp == '\\')
			*bufp = esc2chr(*++sp);
		else
			*bufp = *sp;
	}
	*bufp++ = '\0';

	return buf;
}

char *
descr_encode(const char *s)
{
	const char	*sp;
	const char	*esc;
	char		*buf = NULL;
	size_t		 bufsize = 0;
	char		*bufp;
	size_t		 len;

	/* compute the size of the buffer we need */
	for (sp = s; *sp != '\0'; ++sp) {
		esc = chr2esc(*sp);
		if (esc == NULL)
			bufsize += 1;
		else
			bufsize += strlen(esc);
	}
	bufsize += 2; /* for quotes on both sides */
	bufsize += 1; /* '\0' */

	bufp = buf = xmalloc(bufsize);
	/* encode */
	*bufp++ = '"';
	for (sp = s; *sp != '\0'; ++sp) {
		esc = chr2esc(*sp);
		if (esc == NULL)
			*bufp++ = *sp;
		else {
			len = strlen(esc);
			memcpy(bufp, esc, len);
			bufp += len;
		}
	}
	*bufp++ = '"';
	*bufp++ = '\0';

	return buf;
}

#ifdef DESCR_UNITTEST
#include <stdio.h>
#include <assert.h>
#define UNITTEST(expect, actual)											\
	do {														\
		if (strcmp(expect, actual) != 0) {									\
			fprintf(stderr, "%s, %d: expected „%s“, got „%s“\n", __FILE__, __LINE__, expect, actual);	\
			exit(1);											\
		}													\
	} while (0);
int
main(int argc, char **argv)
{
	char		*enc;
	char		*dec;

	enc = descr_encode("abc");
	UNITTEST("\"abc\"", enc);
	dec = descr_decode(enc);
	UNITTEST("abc", dec);
	xfree(enc);
	xfree(dec);

	enc = descr_encode("a\"\nbc\"");
	UNITTEST("\"a\\\"\\nbc\\\"\"", enc);
	dec = descr_decode(enc);
	UNITTEST("a\"\nbc\"", dec);
	xfree(enc);
	xfree(dec);

	return 0;
}
#endif /* DESCR_UNITTEST */
