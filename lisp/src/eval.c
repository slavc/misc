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

#include <unistd.h>

#include "lisp.h"
#include "eval.h"
#include "mem.h"
#include "util.h"

extern int      dflag;

const char     *ops[] = {
	"quote",
	"atom",
	"eq",
	"car",
	"cdr",
	"cons",
	"cond",
	"label",
	"defun",
	"list",
	"null",
	"and",
	"not",
	"exec",
	NULL
};

struct expr    *(*op_funcs[]) (struct expr *, struct context *) = {
	quote,
	atom,
	eq,
	car,
	cdr,
	cons,
	cond,
	label,
	defun,
	list,
	null,
	and,
	not,
	exec,
	NULL
};

static int      is_atom_t(struct expr * e);
static int      is_empty_list(struct expr * e);

struct expr    *
eval(struct expr * e, struct context * ctx)
{
	int             i, is_root_level;
	struct expr    *pa, *pb, *pc, *pe, **ev, *re;

	if (ctx->is_root_level) {
		is_root_level = 1;
		ctx->is_root_level = 0;
	} else {
		is_root_level = 0;
	}

	dbgprintf("\n>>> EVAL <<<\n");
	if (dflag) {
		peval(e, "@    ");
		pcontext(ctx, "<    ");
	}
	if (e == NULL) {
		dbgprintf(">>> NULL\n");
		re = empty_list();
		if (is_root_level) {
			free_expr(e);
			free(e);
		}
		return re;
	}
	if (e->quoted) {
		dbgprintf(">>> QUOTE\n");
		re = quote(e, ctx);
		if (is_root_level) {
			free_expr(e);
			free(e);
		}
		return re;
	}
	if (e->t == LATOM) {
		dbgprintf(">>> RET ATOM\n");
		if ((pe = search_context(ctx, e->v.atom->v)) != NULL) {
			re = exprs_dup(pe);
			if (is_root_level) {
				free_expr(e);
				free(e);
			}
			return re;
		} else {
			re = exprs_dup(e);
			if (is_root_level) {
				free_expr(e);
				free(e);
			}
			return re;
		}
	} else if (e->t == LLIST) {
		if (e->v.list == NULL)
			return exprs_dup(e);

		if (!e->v.list->v->v.list)
			return empty_list();

		if (e->v.list->v->t == LATOM) {
			for (i = 0; ops[i] != NULL; ++i) {
				if (!strcmp(ops[i], e->v.list->v->v.atom->v))
					break;
			}

			if (ops[i] == NULL) {
				if (pe = search_context(ctx, e->v.list->v->v.atom->v)) {
					free_expr(e->v.list->v);
					e->v.list->v = exprs_dup(pe);
					return eval(e, ctx);
				} else {
					return empty_list();
				}
			}
			dbgprintf(">>> %s\n", ops[i]);
			re = op_funcs[i] (e, ctx);
			if (is_root_level) {
				free_expr(e);
				free(e);
			}
			return re;
		} else if (e->v.list->v->t == LLIST) {
			/* First check if it's a lambda */
			if (is_function_call_expr(e)) {
				dbgprintf(">>> IS FUNCTION CALL\n");
				re = lambda(e, ctx);
				if (is_root_level) {
					free_expr(e);
					free(e);
				}
				return re;
			} else {
				dbgprintf(">>> IS NOT FUNCTION CALL\n");
				pe = eval(e->v.list->v, ctx);
				free_expr(e->v.list->v);
				e->v.list->v = (struct expr *) pe;
				re = eval(e, ctx);
				if (is_root_level) {
					free_expr(e);
					free(e);
				}
				return re;
			}
		}
	}
	return NULL;
}

struct expr    *
atom_t(void)
{
	struct expr    *e;

	e = new_expr(LATOM);
	e->v.atom = new_atom("t");

	return e;
}

struct expr    *
empty_list(void)
{
	return new_expr(LLIST);
}

struct expr    *
quote(struct expr * e, struct context * ctx)
{
	struct expr    *re;
	int             len;

	if (e == NULL)
		return empty_list();

	if (e->quoted) {
		re = exprs_dup(e);
		re->quoted = 0;
	} else {
		if (list_len(e) < 2)
			re = empty_list();
		else
			re = exprs_dup(e->v.list->next->v);

	}
	free_expr(e);
	return re;
}

struct expr    *
atom(struct expr * e, struct context * ctx)
{
	struct expr    *a, *re;
	int             n;

	if (!e || list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);
	if (a->t == LATOM)
		re = atom_t();
	else if (a->t == LLIST && list_len(a) == 0)
		re = atom_t();
	else
		re = empty_list();
	full_free_expr(a);
	free_expr(e);
	return re;
}

struct expr    *
eq(struct expr * e, struct context * ctx)
{
	struct expr    *a, *b, *re;

	if (!e)
		return empty_list();

	if (list_len(e) < 3) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);
	b = eval(e->v.list->next->next->v, ctx);
	if (a->t != b->t) {
		re = empty_list();
	} else if (a->t == LLIST) {
		if (a->v.list == NULL && b->v.list == NULL)
			re = atom_t();
		else
			re = empty_list();
	} else if (!a->v.atom->v || !b->v.atom->v) {
		re = empty_list();
	} else if (!strcmp(a->v.atom->v, b->v.atom->v)) {
		re = atom_t();
	} else {
		re = empty_list();
	}

	full_free_expr(a);
	full_free_expr(b);
	free_expr(e);
	return re;
}

struct expr    *
car(struct expr * e, struct context * ctx)
{
	struct expr    *a, *re;

	if (!e)
		return empty_list();

	if (list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);
	if (a->t != LLIST)
		re = empty_list();
	else {
		if (a->v.list != NULL)
			re = exprs_dup(a->v.list->v);
		else
			re = empty_list();
	}
	full_free_expr(a);
	free_expr(e);
	return re;
}

struct expr    *
cdr(struct expr * e, struct context * ctx)
{
	struct list    *nl;
	struct expr    *a, *re;

	if (!e)
		return empty_list();

	if (list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);
	if (a->t != LLIST || a->v.list == NULL) {
		full_free_expr(a);
		free_expr(e);
		return empty_list();
	}
	nl = list_dup(a->v.list->next);
	if (!nl) {
		re = empty_list();
	} else {
		re = new_expr(LLIST);
		re->v.list = nl;
	}
	full_free_expr(a);
	free_expr(e);
	return re;
}

struct expr    *
cons(struct expr * e, struct context * ctx)
{
	struct expr    *ne, *a, *b, *re;
	struct list    *l;

	if (!e || list_len(e) < 3) {
		dbgprintf("invalid cons expr\n");
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);
	b = eval(e->v.list->next->next->v, ctx);

	if (b->t != LLIST) {
		full_free_expr(a);
		re = empty_list();
	} else {
		re = new_expr(LLIST);
		list_add(re, a);
		for (l = b->v.list; l != NULL; l = l->next)
			list_add(re, exprs_dup(l->v));
		free_list(l);
	}
	full_free_expr(b);
	free_expr(e);
	return re;
}

struct expr    *
cond(struct expr * e, struct context * ctx)
{
	struct expr    *pp, *pe, *t_atom;
	struct list    *l;

	if (!e || list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	for (l = e->v.list->next; l != NULL; l = l->next) {
		if (l->v->t != LLIST || list_len(l->v) < 2) {
			free_expr(e);
			return empty_list();
		}
	}

	for (l = e->v.list->next; l != NULL; l = l->next) {
		pp = eval(l->v->v.list->v, ctx);
		if (pp->t != LATOM || strcmp(pp->v.atom->v, "t") != 0) {
			full_free_expr(pp);
			continue;
		}
		full_free_expr(pp);
		pe = eval(l->v->v.list->next->v, ctx);
		free_expr(e);
		return pe;
	}

	free_expr(e);
	return empty_list();
}

struct expr    *
list(struct expr * e, struct context * ctx)
{
	struct expr    *pe, *re;
	const struct list *l;
	int             i;

	if (!e)
		return empty_list();

	if (list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	re = new_expr(LLIST);
	for (i = 1; (l = get_list_el(e, i)) != NULL; ++i) {
		pe = eval(l->v, ctx);
		list_add(re, pe);
	}
	free_expr(e);
	return re;
}

struct expr    *
lambda(struct expr * e, struct context * context)
{
	int             i;
	const struct list *lp, *la;
	struct expr    *pe, *pr;
	struct context *ctx;

	/* assume e is a valid func call expr */

	ctx = new_context();
	ctx->is_root_level = 0;
	ctx->next = (struct context *) context;
	dbgprintf("created new context\n");
	for (i = 0, lp = e->v.list->v->v.list->next->v->v.list, la = e->v.list->next; lp && la; ++i, lp = lp->next, la = la->next) {
		dbgprintf("evaluating arguments, iter %d\n", i);
		pe = add_to_context(ctx, strdup(lp->v->v.atom->v), eval(la->v, context));
		full_free_expr(pe);
	}
	dbgprintf("eval'ing e expr\n");
	pr = eval(e->v.list->v->v.list->next->next->v, ctx);
	free_expr(e);
	free_context(ctx);
	return pr;
}

struct expr    *
label(struct expr * e, struct context * ctx)
{
	struct expr    *pe;

	if (!e || list_len(e) < 3) {
		dbgprintf("first\n");
		free_expr(e);
		return empty_list();
	}
	if (!e->v.list->v || e->v.list->v->t != LATOM || !e->v.list->v->v.atom || !e->v.list->v->v.atom->v) {
		dbgprintf("second\n");
		free_expr(e);
		return empty_list();
	}
	if (!is_valid_lambda_expr(e->v.list->next->next->v)) {
		free_expr(e);
		return empty_list();
	}
	pe = add_to_context(ctx, strdup(e->v.list->next->v->v.atom->v), exprs_dup(e->v.list->next->next->v));
	full_free_expr(pe);
	free_expr(e);
	return empty_list();
}

struct expr    *
defun(struct expr * e, struct context * ctx)
{
	struct expr    *pe, *pl;
	struct list    *l;

	if (!e || list_len(e) < 4) {
		free_expr(e);
		return empty_list();
	}
	if (!e->v.list->next || !e->v.list->next->v || e->v.list->next->v->t != LATOM || !e->v.list->next->v->v.atom || !e->v.list->next->v->v.atom->v) {
		free_expr(e);
		return empty_list();
	}
	if (!e->v.list->next->next || !is_valid_p_expr(e->v.list->next->next->v)) {
		free_expr(e);
		return empty_list();
	}
	if (!e->v.list->next->next->next || !e->v.list->next->next->next->v) {
		free_expr(e);
		return empty_list();
	}
	pl = new_expr(LLIST);
	pl->v.list = new_list();
	pl->v.list->v = new_expr(LATOM);
	pl->v.list->v->v.atom = new_atom("lambda");
	pl->v.list->next = new_list();
	pl->v.list->next->v = exprs_dup(e->v.list->next->next->v);
	pl->v.list->next->next = new_list();
	pl->v.list->next->next->v = exprs_dup(e->v.list->next->next->next->v);

	pe = add_to_context(ctx, strdup(e->v.list->next->v->v.atom->v), pl);
	full_free_expr(pe);
	free_expr(e);

	return empty_list();
}

struct expr    *
null(struct expr * e, struct context * ctx)
{
	struct expr    *a, *re;

	if (!e || list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);

	if (is_empty_list(a))
		re = atom_t();
	else
		re = empty_list();

	full_free_expr(a);
	return re;
}

struct expr    *
and(struct expr * e, struct context * ctx)
{
	struct expr    *a, *b, *re;

	if (!e || list_len(e) < 3) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);
	b = eval(e->v.list->next->next->v, ctx);

	if (is_atom_t(a) && is_atom_t(b))
		re = atom_t();
	else
		re = empty_list();

	full_free_expr(a);
	full_free_expr(b);
	free_expr(e);
	return re;
}

struct expr    *
not(struct expr * e, struct context * ctx)
{
	struct expr    *a, *re;

	if (!e || list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	a = eval(e->v.list->next->v, ctx);

	if (is_empty_list(a))
		re = atom_t();
	else
		re = empty_list();

	full_free_expr(a);
	free_expr(e);
	return re;
}

struct expr    *
exec(struct expr * e, struct context * ctx)
{
	struct expr    *a, *re;
	struct args     args;
	struct list    *l;

	if (!e || list_len(e) < 2) {
		free_expr(e);
		return empty_list();
	}
	memset(&args, 0, sizeof(args));
	for (l = e->v.list->next; l != NULL; l = l->next) {
		a = eval(l->v, ctx);
		build_argv(a, &args);
		full_free_expr(a);
	}

	re = do_exec(&args);

	free_expr(e);
	free_args(&args);

	return re;
}

int
build_argv(struct expr * e, struct args * a)
{
	struct list    *l;

	if (!e || !a)
		return 0;

	if (e->t == LATOM) {
		dbgprintf("build_argv: an atom\n");

		if (!e->v.atom || !e->v.atom->v)
			return 0;

		if (!a->argv) {
			a->argv = malloc(sizeof(char *) * 2);
			a->argv[0] = strdup(e->v.atom->v);
			a->argv[1] = NULL;
			a->argc = 1;
		} else {
			a->argv = realloc(a->argv, sizeof(char *) * (a->argc + 2));
			a->argv[a->argc++] = strdup(e->v.atom->v);
			a->argv[a->argc] = NULL;
		}

		dbgprintf("build_argv: added \"%s\"\n", a->argv[a->argc - 1]);

		return a->argc;
	} else if (e->t == LLIST) {
		for (l = e->v.list; l != NULL; l = l->next) {
			dbgprintf("build_argv: descending into list\n");
			build_argv(l->v, a);
		}
		return a->argc;
	}
}

struct expr    *
do_exec(struct args * a)
{
	pid_t           pid;
	int             status;

	if (!a || a->argc == 0) {
		dbgprintf("no arguments -- no execvp\n");
		return empty_list();
	}
	dbgprintf("forking...\n");

	if ((pid = fork()) < 0) {
		warn("fork");
		return empty_list();
	} else if (pid > 0) {
		dbgprintf("waiting for child termination\n");
		wait(&status);
		if (status == 0)
			return atom_t();
		else
			return empty_list();
	} else {
		dbgprintf("trying to execvp\n");

		execvp(a->argv[0], a->argv);

		/* if reached, error has occured */
		err(1, "%s", a->argv[0]);

		/* never reached, but just to be sure */
		return NULL;
	}
}

int
search_context_i(const struct context * ctx, const char *key)
{
	int             i;

	for (i = 0; i < ctx->nmap; ++i) {
		if (!strcmp(ctx->map[i].k, key))
			return i;
	}
	return -1;
}

struct expr    *
search_context(const struct context * ctx, const char *k)
{
	int             pos;

	for (ctx; ctx != NULL; ctx = ctx->next) {
		if ((pos = search_context_i(ctx, k)) != -1)
			return ctx->map[pos].v;
	}

	return NULL;
}

int
list_len(const struct expr * e)
{
	int             len = 0;
	const struct list *l;

	if (e->t != LLIST)
		return 0;

	l = e->v.list;
	while (l) {
		++len;
		l = l->next;
	}

	return len;
}

struct list    *
get_list_el(const struct expr * e, int i)
{
	int             n;
	const struct list *l;

	if (!e || e->t != LLIST || i >= list_len(e))
		return NULL;

	for (l = e->v.list, n = 0; l != NULL, n < i; l = l->next, ++n)
		 /* empty */ ;

	return (struct list *) l;
}

struct expr    *
list_add(struct expr * e, const struct expr * ne)
{
	struct list    *l;

	if (!e || e->t != LLIST)
		return NULL;

	if (!e->v.list) {
		e->v.list = new_list();
		e->v.list->v = (struct expr *) ne;
		return e;
	}
	for (l = e->v.list; l->next != NULL; l = l->next)
		 /* empty */ ;
	l->next = new_list();
	l->next->v = (struct expr *) ne;
	return e;
}

int
are_lists_equal(const struct list * a, const struct list * b, struct context * ctx)
{
	const char     *fn = "are_lists_equal";

	while (a && b) {
		if (!are_exprs_equal(a->v, b->v, ctx, 0)) {
			dbgprintf("%s: list not equal (exprs)\n", fn);
			return 0;
		}
		a = a->next;
		b = b->next;
	}

	if (!a ? b : !b ? (void *) 1 : (void *) 0) {
		dbgprintf("%s: one of a or b is null other isnt\n", fn);
		return 0;
	} else {
		dbgprintf("%s: lists equal\n", fn);
		return 1;
	}
}

int
are_exprs_equal(struct expr * a, struct expr * b, struct context * ctx, int do_eval)
{
	struct list    *l;
	struct expr    *e1, *e2;

	const char     *fn = "are_exprs_equal";

	if (a == NULL && b == NULL) {
		dbgprintf("%s: a && b are null\n", fn);
		return 1;
	}
	if (!a ? b : !b ? (void *) 1 : (void *) 0) {
		dbgprintf("%s: one of a or b is null other is not\n", fn);
		return 0;
	}
	if (do_eval) {
		e1 = eval(a, ctx);
		e2 = eval(b, ctx);
	} else {
		e1 = a;
		e2 = b;
	}

	if (e1->t != e2->t) {
		dbgprintf("%s: types not equal\n", fn);
		return 0;
	}
	if (e1->t == LATOM) {
		dbgprintf("strcmping %s %s\n", e1->v.atom->v, e2->v.atom->v);
		return !strcmp(e1->v.atom->v, e2->v.atom->v);
	} else if (e1->t == LLIST) {
		return are_lists_equal(e1->v.list, e2->v.list, ctx);
	}
}

int
is_valid_p_expr(const struct expr * e)
{
	const struct list *l;

	if (!e || e->t != LLIST || e->v.list == NULL)
		return 0;
	for (l = e->v.list; l != NULL; l = l->next) {
		if (l->v == NULL || l->v->t != LATOM)
			return 0;
	}
	return 1;
}

struct expr    *
add_to_context(struct context * ctx, const char *k, const struct expr * e)
{
	int             i;
	struct expr    *pr;

	if (!e)
		return NULL;

	if ((i = search_context_i(ctx, k)) == -1) {
		++ctx->nmap;
		ctx->map = realloc(ctx->map, sizeof(*ctx->map) * ctx->nmap);
		i = ctx->nmap - 1;
		pr = NULL;
	} else {
		/* free */
		pr = ctx->map[i].v;
	}

	ctx->map[i].k = (char *) k;
	ctx->map[i].v = (struct expr *) e;

	return pr;
}

int
is_valid_lambda_expr(const struct expr * e)
{
	struct list    *l;

	if (!e || e->t != LLIST || list_len(e) < 3)
		return 0;

	if (!e->v.list->v || e->v.list->v->t != LATOM || !e->v.list->v->v.atom || !e->v.list->v->v.atom->v || strcmp(e->v.list->v->v.atom->v, "lambda") != 0)
		return 0;
	if (!is_valid_p_expr(e->v.list->next->v))
		return 0;
	if (!e->v.list->next->next->v)
		return 0;

	return 1;
}

int
is_function_call_expr(const struct expr * e)
{
	if (!e || e->t != LLIST || e->v.list == NULL)
		return 0;

	if (!e->v.list->v || e->v.list->v->t != LLIST || e->v.list->v->v.list == NULL || e->v.list->v->v.list->v == NULL || e->v.list->v->v.list->v->t != LATOM || e->v.list->v->v.list->v->v.atom == NULL || e->v.list->v->v.list->v->v.atom->v == NULL)
		return 0;

	if (strcmp(e->v.list->v->v.list->v->v.atom->v, "lambda") != 0)
		return 0;

	if (list_len(e->v.list->v) < 3 || e->v.list->v->v.list->next->v == NULL || e->v.list->v->v.list->next->v->t != LLIST)
		return 0;

	return 1;
}

static int
is_atom_t(struct expr * e)
{
	if (!e || e->t != LATOM || !e->v.atom || !e->v.atom->v)
		return 0;
	return strcmp(e->v.atom->v, "t") == 0;
}

static int
is_empty_list(struct expr * e)
{
	return e && e->t == LLIST && !e->v.list;
}
