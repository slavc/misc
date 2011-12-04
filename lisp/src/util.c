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
#include "util.h"

extern int      dflag;

int
dbgprintf(const char *fmt,...)
{
	int             n;
	va_list         ap;

	if (!dflag)
		return 0;
	va_start(ap, fmt);
	n = vprintf(fmt, ap);
	va_end(ap);

	return n;
}

char           *
str_append(const char *a, const char *b)
{
	char           *buf;
	size_t          size;

	size = strlen(a) + strlen(b) + 1;
	buf = malloc(size);
	strcpy(buf, a);
	strcat(buf, b);
	return buf;
}

void
pcontext(const struct context * ctx, const char *indent)
{
	int             i;
	char           *buf;

	if (!ctx)
		return;

	printf("%sContext {\n", indent);

	buf = str_append(indent, "    ");
	for (i = 0; i < ctx->nmap; ++i) {
		printf("%skey = %s\n%sval = \n", buf, ctx->map[i].k, buf);
		peval(ctx->map[i].v, buf);
		printf("%s----\n", buf);
	}
	pcontext(ctx->next, buf);
	Free(buf);
	printf("%s}\n", indent);
}

void
peval(const struct expr * e, const char *indent)
{
	struct list    *l;
	char           *buf;
	static const char *qv[] = {"", " <q>"};
	const char     *q;

	if (e == NULL) {
		printf("%sNULL\n", indent);
		return;
	}
	q = qv[abs(e->quoted % 2)];

	if (e->t == LATOM) {
		printf("%sATOM%s %s\n", indent, q, (e->v.atom ? e->v.atom->v : "[null]"));
	} else if (e->t == LLIST) {
		if (e->v.list == NULL) {
			printf("%sEMPTY_LIST%s\n", indent, q);
		} else {
			printf("%sLIST%s {\n", indent, q);
			buf = str_append(indent, "    ");
			for (l = e->v.list; l != NULL; l = l->next)
				peval(l->v, buf);
			Free(buf);
			printf("%s}\n", indent);
		}
	} else {
		printf("%sUNKNOWN EXPR TYPE%s\n", indent, q);
	}
}

void
print_atom(struct expr * e, int depth)
{
	if (!e || !e->v.atom || !e->v.atom->v) {
		printf("(null)");
		return;
	}
	printf("%s%s%s", depth ? " " : "", e->quoted ? "'" : "", e->v.atom->v);
}

void
print_list(struct expr * e, int depth)
{
	int             i;
	const struct list *l;

	if (!e) {
		printf("(null)");
		return;
	}
	printf("%s%s(", depth ? " " : "", e->quoted ? "'" : "");
	for (l = e->v.list; l != NULL; l = l->next)
		print_expr(l->v, l != e->v.list);
	printf(")");
}

void
print_expr(struct expr * e, int depth)
{
	const char     *fmt;

	if (!e) {
		printf("(null)");
		return;
	}
	if (e->t == LATOM)
		print_atom(e, depth);
	else if (e->t == LLIST)
		print_list(e, depth);
	else
		printf("(expression of unknown type)");
}

char           *
load_file_fp(FILE * fp)
{
	char           *buf = NULL;
	size_t          buf_size = 0;
	size_t          n = 0;
	enum {
	chunk_size = 8};
	char            chunk[chunk_size];

	while ((n = fread(chunk, 1, chunk_size, fp)) > 0) {
		buf = realloc(buf, buf_size + n + 1);
		memcpy(buf + buf_size, chunk, n);
		buf_size += n;
		buf[buf_size] = '\0';
	}

	if (buf_size == 0) {
		buf = malloc(1);
		*buf = '\0';
	}
	return buf;
}

char           *
load_file(const char *filename)
{
	FILE           *fp;
	char           *buf;

	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;
	else {
		buf = load_file_fp(fp);
		fclose(fp);
		return buf;
	}
}
