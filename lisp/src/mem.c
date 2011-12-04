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
#include "mem.h"

struct expr    *
new_expr(int t)
{
	struct expr    *e;

	e = calloc(1, sizeof(*e));
	e->t = t;
	return e;
}

struct list    *
new_list(void)
{
	return calloc(1, sizeof(struct list));
}

struct atom    *
new_atom(const char *s)
{
	struct atom    *a;
	a = calloc(1, sizeof(struct atom));
	if (s)
		a->v = strdup(s);
	return a;
}

struct context *
new_context(void)
{
	return calloc(1, sizeof(struct context));
}

struct list    *
list_dup(const struct list * l)
{
	struct list    *nl;

	if (l == NULL)
		return NULL;

	nl = new_list();
	nl->v = exprs_dup(l->v);
	nl->next = list_dup(l->next);
	return nl;
}

struct atom    *
atom_dup(const struct atom * a)
{
	struct atom    *na;

	if (a == NULL)
		return NULL;
	else
		return new_atom(a->v);
}

struct expr    *
exprs_dup(const struct expr * e)
{
	struct expr    *ne;

	if (e == NULL)
		return NULL;

	ne = new_expr(e->t);
	ne->quoted = e->quoted;

	if (e->t == LATOM) {
		ne->v.atom = atom_dup(e->v.atom);
	} else if (e->t == LLIST) {
		ne->v.list = list_dup(e->v.list);
	}
	return ne;
}

void           *
free_atom(struct atom * a)
{
	if (!a)
		return NULL;
	if (a->v)
		Free(a->v);
	Free(a);
	return NULL;
}

void           *
free_list(struct list * l)
{
	if (!l)
		return NULL;
	if (l->next)
		l->next = free_list(l->next);
	l->v = free_expr(l->v);
	Free(l);
	return NULL;
}

void           *
free_expr(struct expr * e)
{
	if (!e)
		return NULL;
	if (e->t == LATOM)
		e->v.atom = free_atom(e->v.atom);
	else if (e->t == LLIST)
		e->v.list = free_list(e->v.list);
	/* Free(e); */
	return NULL;
}

void           *
full_free_expr(struct expr * e)
{
	free_expr(e);
	Free(e);
	return NULL;
}

void           *
free_context(struct context * ctx)
{
	int             i;

	if (!ctx)
		return NULL;

	for (i = 0; i < ctx->nmap; ++i) {
		Free(ctx->map[i].k);
		free_expr(ctx->map[i].v);
	}
	ctx->nmap = 0;
	Free(ctx->map);
	return NULL;
}

void           *
free_context_r(struct context * ctx)
{
	int             i;

	if (!ctx)
		return NULL;
	if (ctx->next)
		free_context_r(ctx->next);
	free_context(ctx);
	return NULL;
}

void
free_args(struct args * a)
{
	int             i;

	if (!a || a->argc == 0)
		return;

	for (i = 0; i < a->argc; ++i)
		Free(a->argv[i]);
}
