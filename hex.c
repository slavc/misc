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

/*
 * hex
 *
 * Print args in binary, character, decimal, hexadecimal and octal notation.
 */

#include <unistd.h>
#include <err.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define NELEMS(array) (sizeof(array)/sizeof((array)[0]))

static void			 hex(const char *);
unsigned long long		 read_bin(const char *);
unsigned long long		 read_dec(const char *);
unsigned long long		 read_hex(const char *);
unsigned long long		 read_oct(const char *);
static unsigned long long	 read_num(const char *, int, const char *);
static void			 usage(void);
static int			 digit2int(char);
static void			 print_bin(unsigned long long);
static void			 print_chr(unsigned long long);
static void			 print_dec(unsigned long long);
static void			 print_hex(unsigned long long);
static void			 print_oct(unsigned long long);
static void			 print_num(unsigned long long, int, const char *);
static char			 int2digit(int i);

static const char	 digits[] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f',
};
static const char	 digit_values[] = {
	0,   1,  2,  3,
	4,   5,  6,  7,
	8,   9, 10, 11,
	12, 13, 14, 15,
};

int
main(int argc, char **argv)
{
	while (++argv, --argc)
		hex(*argv);
	return 0;
}

static void
hex(const char *s)
{
	unsigned long long i;

	switch (s[0]) {
	case 'b':
	case 'B':
		i = read_bin(s + 1);
		break;
	case 'x':
	case 'X':
		i = read_hex(s + 1);
		break;
	case '0':
		if (s[1] == '\0')
			i = read_dec(s);
		else if (s[1] == 'x' || s[1] == 'X')
			i = read_hex(s + 2);
		else
			i = read_oct(s + 1);
		break;
	default:
		if (isdigit(*s))
			i = read_dec(s);
		else
			usage();
		break;
	}
	print_bin(i);
	print_chr(i);
	print_dec(i);
	print_hex(i);
	print_oct(i);
}

unsigned long long
read_bin(const char *s)
{
	return read_num(s, 2, "binary");
}

unsigned long long
read_dec(const char *s)
{
	return read_num(s, 10, "decimal");
}

unsigned long long
read_hex(const char *s)
{
	return read_num(s, 16, "hexadecimal");
}

unsigned long long
read_oct(const char *s)
{
	return read_num(s, 8, "octal");
}

static void
usage(void)
{
	printf("usage: hex <number>\n");
	exit(EXIT_FAILURE);
}

static unsigned long long
read_num(const char *s, int base, const char *name)
{
	const char		*p = s;
	unsigned long long	 i = 0;

	while (*p != '\0' && isxdigit(*p)) {
		i *= base;
		i += (unsigned) digit2int(*p);
		++p;
	}
	if (*p != '\0')
		errx(1, "%s: not a %s number", s, name ? name : "valid");
	return i;
}

static int
digit2int(char c)
{
	int	 i;

	c = tolower(c);
	for (i = 0; i < NELEMS(digits); ++i)
		if (digits[i] == c)
			return digit_values[i];
	return -1;
}


static void
print_bin(unsigned long long n)
{
	print_num(n, 2, "b%s\n");
}

static void
print_chr(unsigned long long n)
{
	if (n < 256)
		printf("'%c'\n", (char) n);
	else
		printf("U+%04x\n", (unsigned int) n);
}

static void
print_dec(unsigned long long n)
{
	print_num(n, 10, "%s\n");
}

static void
print_hex(unsigned long long n)
{
	print_num(n, 16, "0x%s\n");
}

static void
print_oct(unsigned long long n)
{
	print_num(n, 8, "0%s\n");
}

static void
print_num(unsigned long long n, int base, const char *fmt)
{
	const size_t	 bufsize = sizeof(n) * CHAR_BIT + 1;
	char		 buf[bufsize], *p;
	int		 rem;

	p = buf + bufsize - 1;
	*p = '\0';

	while (n) {
		rem = n % base;
		*--p = int2digit(rem);
		n /= base;
	}

	printf(fmt, *p == '\0' ? "0" : p);
}

static char
int2digit(int i)
{
	if (i < 0 || i >= NELEMS(digits))
		return '?';
	return toupper(digits[i]);
}
