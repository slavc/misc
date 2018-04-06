/* Copyright (c) 2014, Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
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
#include <sys/queue.h>
#include <unistd.h>
#include <pwd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#define NELEMS(arr)	(sizeof(arr)/sizeof(*(arr)))

/*
 * Formatting element.
 */
struct elem {
	TAILQ_ENTRY(elem)	 link;
	enum {
		TEXT,
		SPRING
	}	 type;
	char	*buf;
};
TAILQ_HEAD(elems, elem)	elems = TAILQ_HEAD_INITIALIZER(elems);

/*
 * Text attributes.
 */
struct attr {
	unsigned	 bold:1;
	unsigned	 faint:1;
	unsigned	 italic:1;
	unsigned	 underline:1;
	unsigned	 reverse:1;
	int		 fg;		/* for fg and bg, -1 means none */
	int		 bg;
};

static int		 window_width;
static const char	*colors[] = {	/* table of colors in the order of their indexes */
	"black",
	"red",
	"green",
	"yellow",
	"blue",
	"magenta",
	"cyan",
	"white",
	"", /* reserved */ 
	"default",
	NULL
};

#define ATTR_BOLD	1
#define ATTR_FAINT	2
#define ATTR_ITALIC	3
#define ATTR_UNDERLINE	4
#define ATTR_REVERSE	7
#define ATTR_RESET	0
#define ATTR_END	-1

static void *
xcalloc(size_t n, size_t size)
{
	void	*ptr;

	ptr = calloc(n, size);
	if (ptr == NULL)
		err(EXIT_FAILURE, "calloc");
	return ptr;
}

static int
color(const char *col, int base)
{
	const char	**p;
	int		 i;

	i = atoi(col);
	if (i != 0 && i >= base && i < base + NELEMS(colors))
		return i;

	for (p = colors, i = 0; *p != NULL; ++p, ++i)
		if (!strcmp(*p, col))
			return i + base;

	errx(EXIT_FAILURE, "%s: invalid color", col);
}

static int
fg_color(const char *col)
{
	return color(col, 30);
}

static int
bg_color(const char *col)
{
	return color(col, 40);
}

static void
put_attr(int attr)
{
	static unsigned	 n = 0; /* num attrs put so far */

	if (attr == ATTR_RESET) {
		if (n > 0)
			printf("\033[0m");
		n = 0;
	} else if (attr == ATTR_END) {
		if (n > 0)
			putchar('m');
	} else {
		if (n++ == 0)
			printf("\033[%d", attr);
		else
			printf(";%d", attr);
	}
}

static void
attrs_put(struct attr attr)
{
	if (attr.bold)
		put_attr(ATTR_BOLD);
	if (attr.faint)
		put_attr(ATTR_FAINT);
	if (attr.italic)
		put_attr(ATTR_ITALIC);
	if (attr.underline)
		put_attr(ATTR_UNDERLINE);
	if (attr.reverse)
		put_attr(ATTR_REVERSE);
	if (attr.fg != -1)
		put_attr(attr.fg);
	if (attr.bg != -1)
		put_attr(attr.bg);
	put_attr(ATTR_END);
}

static void
attrs_reset(void)
{
	put_attr(ATTR_RESET);
}

static void
get_window_width(void)
{
	const char	*s;

	s = getenv("COLUMNS");
	if (s == NULL)
		errx(EXIT_FAILURE, "COLUMNS is not set");
	window_width = atoi(s);
}

static struct elem *
elem_new(int type, ...)
{
	struct elem	*p;
	va_list		 ap;
	const char	*start, *end;

	p = xcalloc(1, sizeof *p);
	p->type = type;

	if (type == TEXT) {
		va_start(ap, type);
		start = va_arg(ap, const char *);
		end = va_arg(ap, const char *);
		va_end(ap);

		p->buf = xcalloc(1, end - start + 1);
		memcpy(p->buf, start, end - start);
	}

	return p;
}

static void
prompt(struct attr attr, const char *fmt, const char *springpat)
{
	const char	*textp;
	struct elem	*elem;
	size_t		 textlen;	/* total text lenght */
	size_t		 springlen;	/* total spring lenght */
	size_t		 nsprings;
	size_t		 rem;
	size_t		 len;
	size_t		 i;
	size_t		 springpatlen;
	
	for (textp = fmt; /* empty */; ++fmt) {
		if ((*fmt == '%' && fmt[1] != '%') || *fmt == '\0') {
			if (textp != fmt && *textp != '\0') {
				elem = elem_new(TEXT, textp, fmt);
				TAILQ_INSERT_TAIL(&elems, elem, link);
			}
			textp = fmt + 1;

			if (*fmt != '\0') {
				elem = elem_new(SPRING);
				TAILQ_INSERT_TAIL(&elems, elem, link);
			}

			if (*fmt == '\0')
				break;
		}
	}

	textlen = 0;
	nsprings = 0;
	TAILQ_FOREACH(elem, &elems, link) {
		if (elem->type == TEXT)
			textlen += strlen(elem->buf);
		else
			++nsprings;
	}

	get_window_width();
	springlen = window_width - textlen;

	springpatlen = strlen(springpat);
	i = 0;
	attrs_put(attr);
	TAILQ_FOREACH(elem, &elems, link) {
		if (elem->type == TEXT)
			printf("%s", elem->buf);
		else {
			rem = springlen % nsprings;
			len = springlen / nsprings;
			len += rem;
			springlen -= rem;

			while (len--)
				putchar(springpat[i++ % springpatlen]);
		}
	}
	attrs_reset();

	while ((elem = TAILQ_FIRST(&elems)) != NULL) {
		TAILQ_REMOVE(&elems, elem, link);
		if (elem->type == TEXT)
			free(elem->buf);
		free(elem);
	}
}

static void
usage(void)
{
	const char	**p;
	int		 i;

	printf(
	    "usage: prompt [-B] [-f <fg>] [-b <bg] [-p <txt>] <fmt>\n"
	    "Draw a ruler-style command prompt.\n"
	    "Export COLUMNS environment variable before using.\n"
	    "  -B -- bold\n"
	    "  -F -- faint\n"
	    "  -I -- italic\n"
	    "  -U -- underline\n"
	    "  -R -- reverse\n"
	    "  -f -- foreground color (name or base-0 index)\n"
	    "  -b -- background color (name or base-0 index)\n"
	    "  -p -- pattern of text to fill springs with\n"
	    "  <fmt> -- text to display. '%%' character acts as a spring,\n"
	    "           escape by doubling.\n"
	);
	printf("Colors:\n");
	for (p = colors, i = 0; *p != NULL; ++p, ++i)
		if (**p != '\0')
			printf(" %d %s\n", i, *p);
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern char	*optarg;
	extern int	 optind;
	struct attr	 attr;
	const char	*fmt = NULL;
	const char	*springpat = " ";

	memset(&attr, 0, sizeof attr);
	attr.fg = -1;
	attr.bg = -1;
	while ((ch = getopt(argc, argv, "b:hf:p:BFIUR")) != -1) {
		switch (ch) {
		case 'p':
			springpat = optarg;
			if (strlen(optarg) == 0)
				errx(EXIT_FAILURE, "invalid fill pattern");
			break;
		case 'f':
			attr.fg = fg_color(optarg);
			break;
		case 'b':
			attr.bg = bg_color(optarg);
			break;
		case 'B':
			attr.bold = true;
			break;
		case 'F':
			attr.faint = true;
			break;
		case 'I':
			attr.italic = true;
			break;
		case 'U':
			attr.underline = true;
			break;
		case 'R':
			attr.reverse = true;
			break;
		case 'h':
			usage();
			return EXIT_SUCCESS;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 1)
		fmt = *argv;
	else {
		usage();
		return EXIT_FAILURE;
	}

	prompt(attr, fmt, springpat);

	return EXIT_SUCCESS;
}
