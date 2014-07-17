/*
 * Copyright (c) 2014 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

#include <sys/types.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <err.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define FG_BASE	30
#define BG_BASE	40

static int	 bflag; /* bold */
static int	 iflag; /* ignore case */
static int	 fflag; /* force color output */

static void		*xrealloc(void *, size_t);
static void		 usage(int);
static bool		 istty(int);
static int		 passthru(void);
static int		 color(char *, const char *);
static const char	*strregerror(int);
static void		 parsecolor(char *, int *, int *);
static const char	*fmtesc(int, int, int);
static int		 coloridx(const char *);
static size_t		 ins(char **, size_t *, const char *, off_t);

/*
 * Color text that matches a regexp.
 */
int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;

	while ((ch = getopt(argc, argv, "bfhi")) != -1) {
		switch (ch) {
		case 'b':
			++bflag;
			break;
		case 'f':
			++fflag;
			break;
		case 'i':
			++iflag;
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			/* NOTREACHED */
		case '?':
		default:
			usage(EXIT_FAILURE);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage(EXIT_FAILURE);
	if (istty(STDOUT_FILENO) || fflag)
		return color(argv[0], argv[1]);
	else
		return passthru();
}

static void *
xrealloc(void *p, size_t size)
{
	void	*q;

	q = realloc(p, size);
	if (q == NULL)
		err(EXIT_FAILURE, NULL);
	return q;
}

static void
usage(int exitcode)
{
	extern char	*__progname;

	printf("usage: %s [-b] [-i] [<fg>]:[<bg>] <pattern>\n", __progname);
	printf("Read from stdin, colorize text matching <pattern>, output to stdout.\n");
	printf("	-b -- bold\n");
	printf("	-f -- force color output even when stdout is not terminal\n");
	printf("	-i -- ignore case\n");
	printf("	<fg>, <bg> -- foreground and background colors\n");
	printf("	<pattern> -- extended regular expression\n");
	exit(exitcode);
}

static bool
istty(int fd)
{
	int	 error;
	int	 num;

	error = ioctl(fd, TIOCOUTQ, &num);
	if (error && errno == ENOTTY)
		return false;
	else
		return true;
}

static int
passthru(void)
{
	char		 buf[8192];
	const size_t	 bufsize = sizeof buf;
	ssize_t		 nread;
	ssize_t		 nwrote;

	while ((nread = read(STDIN_FILENO, buf, bufsize)) > 0) {
		nwrote = write(STDOUT_FILENO, buf, nread);
		if (nwrote != nread)
			err(EXIT_FAILURE, "short write");
	}
	if (nread < 0)
		err(EXIT_FAILURE, "read");

	return EXIT_SUCCESS;
}

static int
color(char *color, const char *pattern)
{
	regmatch_t	 match;
	regex_t		 re;
	char		*line;
	size_t		 linesz;
	size_t		 len;
	const char	*esccolor;
	const char	 escreset[] = "\e[0m";
	int		 fg;
	int		 bg;
	int		 rc;
	regoff_t	 off;

	line = NULL;
	linesz = 0;

	parsecolor(color, &fg, &bg);
	esccolor = fmtesc(fg, bg, bflag);

	rc = regcomp(&re, pattern, REG_EXTENDED | (iflag ? REG_ICASE : 0));
	if (rc != 0)
		errx(EXIT_FAILURE, "``%s'': %s", pattern, strregerror(rc));

	errno = 0;
	while (getline(&line, &linesz, stdin) != -1) {
		for (off = 0; (rc = regexec(&re, line + off, 1, &match, 0)) != REG_NOMATCH; /* empty */ ) {
			if (rc != 0)
				errx(EXIT_FAILURE, "regexec: %s", strregerror(rc));
			match.rm_eo += ins(&line, &linesz, esccolor, match.rm_so + off);
			off += ins(&line, &linesz, escreset, match.rm_eo + off);
			off += match.rm_eo;
		}
		len = strlen(line);
		if (write(STDOUT_FILENO, line, len) != len)
			err(EXIT_FAILURE, "write");
		errno = 0;
	}
	if (errno != 0)
		err(EXIT_FAILURE, "getline");
	regfree(&re);
	free(line);

	return EXIT_SUCCESS;
}

static void
parsecolor(char *col, int *fg, int *bg)
{
	char	*p;

	p = strchr(col, ':');
	if (p == NULL) {
		*fg = coloridx(col);
		*bg = -1;
	} else {
		*p++ = '\0';
		*fg = coloridx(col);
		*bg = coloridx(p);
	}
}

static const char *
strregerror(int code)
{
	/* reg{comp,exec} error code --> description */
	static const struct {
		int		 code;
		const char	*descr;
	} tab[] = {
		{ REG_NOMATCH,     "regexec() failed to match"},
		{ REG_BADPAT,      "invalid regular expression"},
		{ REG_ECOLLATE,    "invalid collating element"},
		{ REG_ECTYPE,      "invalid character class"},
		{ REG_EESCAPE,     "\\ applied to unescapable character"},
		{ REG_ESUBREG,     "invalid backreference number"},
		{ REG_EBRACK,      "brackets [ ] not balanced"},
		{ REG_EPAREN,      "parentheses ( ) not balanced"},
		{ REG_EBRACE,      "braces { } not balanced"},
		{ REG_BADBR,       "invalid repetition count(s) in { }"},
		{ REG_ERANGE,      "invalid character range in [ ]"},
		{ REG_ESPACE,      "ran out of memory"},
		{ REG_BADRPT,      "?, *, or + operand invalid"},
#ifdef OpenBSD
		{ REG_ASSERT,      "``can't happen'' --you found a bug"},
		{ REG_INVARG,      "invalid argument, e.g., negative-length string"},
		{ REG_EMPTY,       "empty (sub)expression"},
#endif
		{ 0, NULL}
	}, *p;

	for (p = tab; p->descr != NULL; ++p)
		if (code == p->code)
			return p->descr;
	return "unknown error";
}

static const char *
fmtesc(int fg, int bg, int bold)
{
	static char	 buf[sizeof "\e[1;30;40m"];
	char		*writep;
	int		 addcolon;
	int		 n;

	writep = buf;
	addcolon = 0;

#define PUTSEMICOL()					\
	do {						\
		if (addcolon++)				\
			*writep++ = ';';		\
	} while (0)
#define APPEND(...)					\
	do {						\
		n = sprintf(writep, __VA_ARGS__);	\
		if (n == -1)				\
			err(1, "sprintf");		\
		writep += n;				\
	} while (0)

	APPEND("\e[");
	if (bold) {
		PUTSEMICOL();
		APPEND("1");
	}
	if (fg > -1) {
		PUTSEMICOL();
		APPEND("%d", FG_BASE + fg);
	}
	if (bg > -1) {
		PUTSEMICOL();
		APPEND("%d", BG_BASE + bg);
	}
	APPEND("m");

	return buf;

#undef PUTSEMICOL
#undef APPEND
}

static int
coloridx(const char *color)
{
	const char	*tab[] = {
		"black",
		"red",
		"green",
		"yellow",
		"blue",
		"magenta",
		"cyan",
		"white",
		NULL
	};
	int		 i;

	for (i = 0; tab[i] != NULL; ++i)
		if (!strcmp(color, tab[i]))
			return i;
	return -1;
}

/*
 * Insert txt into buffer s of size sz before pos.
 * Return strlen of txt.
 */
static size_t
ins(char **s, size_t *sz, const char *txt, off_t pos)
{
	size_t	 txtlen;

	txtlen = strlen(txt);
	*s = xrealloc(*s, *sz + txtlen);
	memmove(*s + pos + txtlen, *s + pos, *sz - pos);
	memcpy(*s + pos, txt, txtlen);
	*sz += txtlen;
	return txtlen;
}
