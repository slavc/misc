/*
 * Copyright (c) 2009 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

#include "lisp.h"
#include "parse.h"
#include "mem.h"

static int      is_empty_list(const char *e);

char           *
find_space(const char *s)
{
	while (!isspace(*s) && *s != '\0')
		++s;
	return (char *) s;
}

char           *
skip_space(const char *s)
{
	while (isspace(*s))
		++s;
	return (char *) s;
}

char           *
find_matching_parenthesis(const char *s)
{
	int             lev = 0;

	dbgprintf("find_matching_parenthesis: %s\n", s);

	if (*s == '(')
		++s;
	while (*s != '\0') {
		if (*s == '(')
			++lev;
		else if (*s == ')') {
			if (lev == 0)
				return (char *) s + 1;
			else {
				--lev;
			}
		}
		++s;
	}
	return (char *) s;
}

char           *
find_expr_end(const char *e)
{
	dbgprintf("find_expr_end: %s\n", e);

	if (*e == '\'')
		++e;
	e = skip_space(e);
	if (*e == '(') {
		e = find_matching_parenthesis(e);
		return (char *) (*e == ')' ? e + 1 : e);
	} else {
		while (!isspace(*e) && *e != '(' && *e != ')' && *e != '\0')
			++e;
		return (char *) e;
	}
}

char           *
next_expr(const char *e)
{
	dbgprintf("next_expr: %s\n", e);
	return (char *) (skip_space(find_expr_end(e)));
}

int
is_atom(const char *e)
{
	return !isspace(*e);
}

int
is_list(const char *e)
{
	return *e == '(';
}

static int
is_empty_list(const char *e)
{
	if (*e == '(')
		++e;
	while (*e != '\0' && *e != ')') {
		if (!isspace(*e))
			return 0;
		++e;
	}
	return 1;
}

char           *
expr_dup(const char *e)
{
	char           *buf;
	char           *end;
	size_t          size;

	end = find_expr_end(e);
	size = end - e + 1;
	buf = malloc(size);
	if (size > 1)
		strncpy(buf, e, size - 1);
	buf[size - 1] = '\0';
	return buf;
}


struct list    *
parse_list(const char *e)
{
	dbgprintf("parse_list: %s\n", e);

	char           *p;
	char           *buf;
	struct list    *first = NULL, *list;
	int             first_iter;

	if (is_empty_list(e))
		return NULL;

	first = list = new_list();
	p = (char *) e + 1;
	first_iter = 1;
	while (*p != ')' && *p != '\0') {
		if (!first_iter) {
			list->next = new_list();
			list = list->next;
		}
		buf = expr_dup(p);
		list->v = parse_expr(buf);
		Free(buf);
		p = next_expr(p);
		if (first_iter)
			first_iter = 0;
	}

	return first;
}


struct atom    *
parse_atom(const char *e)
{
	dbgprintf("parse_atom: %s\n", e);

	struct atom    *atom;

	if (isspace(*e) || *e == '(' || *e == ')' || *e == '\0')
		return NULL;
	atom = new_atom(e);

	return atom;
}

struct expr    *
parse_expr(const char *e)
{
	dbgprintf("parse_expr: %s\n", e);

	struct expr    *expr;

	e = skip_space(e);

	expr = new_expr(LINVALID);

	if (*e == '\'') {
		expr->quoted = 1;
		++e;
	}
	if (*e == '(') {
		expr->t = LLIST;
		expr->v.list = parse_list(e);
	} else {
		expr->t = LATOM;
		expr->v.atom = parse_atom(e);
	}

	return expr;
}

char           *
next_code_chunk(const char *s)
{
	if (!s)
		return NULL;

	++s;
	while (*s != '\n' && *s != '\0')
		++s;

	if (*s == '\n')
		++s;
	return (char *) s;
}

char           *
find_comment(const char *s)
{
	if (!s)
		return NULL;

	while (*s != ';' && *s != '#' && *s != '\0')
		++s;

	if (*s == ';' || *s == '#')
		return (char *) s;
	else
		return NULL;
}

char           *
remove_comments(char *buffer)
{
	char           *buf = NULL;
	size_t          bufsize = 0;
	size_t          len = 0;
	char           *p, *s, *e;

	if (!buffer)
		return NULL;

	for (s = buffer; s != NULL && *s != '\0'; s = next_code_chunk(e)) {
		if (e = find_comment(s))
			*e = '\0';
		if ((len = strlen(s)) == 0)
			continue;
		buf = realloc(buf, bufsize + len + (bufsize ? 1 : 2));
		memcpy(buf + (bufsize ? bufsize - 1 : 0), s, len);
		bufsize += len + (bufsize ? 1 : 2);
		buf[bufsize - 2] = ' ';
		buf[bufsize - 1] = '\0';
	}

	if (!bufsize) {
		buf = malloc(sizeof(char));
		*buf = '\0';
	} else {
		buf[bufsize - 2] = '\0';
	}

	return buf;
}

char           *
load_lisp_source_file(const char *filename)
{
	char           *buf1, *buf2;

	if ((buf1 = load_file(filename)) == NULL)
		return NULL;

	buf2 = remove_comments(buf1);
	free(buf1);
	return buf2;
}
