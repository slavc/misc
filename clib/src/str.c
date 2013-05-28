/*
 * Copyright (c) 2013 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xmem.h>

#define PREALLOC_SIZE	127

char *
str_prepend(char *s, const char *fmt, ...)
{
	int		 n = 0;
	const int	 preallocsize = PREALLOC_SIZE;
	int		 bufsize = 0;
	char		*buf = NULL;
	va_list		 ap;
	size_t		 slen;
	size_t		 ssize;

	n = preallocsize;
	do {
		bufsize = n + 1;
		buf = xrealloc(buf, bufsize);
		va_start(ap, fmt);
		n = vsnprintf(buf, bufsize, fmt, ap);
		va_end(ap);
		if (n < 0)
			err(1, "%s: vsnprintf", __func__);
	} while (n >= bufsize);

	if (s == NULL)
		slen = 0;
	else
		slen = strlen(s);
	ssize = slen + n + 1;
	s = xrealloc(s, ssize);
	memmove(s + n, s, slen);
	memcpy(s, buf, n);
	s[ssize - 1] = '\0';

	xfree(buf);

	return s;
}

int
str_is_space(const char *s)
{
	while (isspace(*s))
		++s;
	if (*s == '\0')
		return 1;
	else
		return 0;
}

#ifdef TEST_STR
int
main(int argc, char **argv)
{
	char	*s = NULL;

	s = str_prepend(s, "%s", "sys");
	s = str_prepend(s, "%s.", "ctl");
	s = str_prepend(s, "%s.", "abc");

	if (strcmp(s, "abc.ctl.sys") == 0)
		return 0;
	else
		errx(1, "str_prepend unittests failed");
}
#endif
