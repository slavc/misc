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

#ifndef EVAL_H
#define EVAL_H

#include "lisp.h"

struct args {
	int             argc;
	char          **argv;
};

struct expr    *eval(struct expr * e, struct context * ctx);
struct expr    *atom_t(void);
struct expr    *empty_list(void);
struct expr    *quote(struct expr * e, struct context * ctx);
struct expr    *atom(struct expr * e, struct context * ctx);
struct expr    *eq(struct expr * e, struct context * ctx);
struct expr    *car(struct expr * e, struct context * ctx);
struct expr    *cdr(struct expr * e, struct context * ctx);
struct expr    *cons(struct expr * e, struct context * ctx);
struct expr    *cond(struct expr * e, struct context * ctx);
struct expr    *list(struct expr * e, struct context * ctx);
struct expr    *lambda(struct expr * e, struct context * context);
struct expr    *label(struct expr * e, struct context * ctx);
struct expr    *defun(struct expr * e, struct context * ctx);
struct expr    *null(struct expr * e, struct context * ctx);
struct expr    *and(struct expr * e, struct context * ctx);
struct expr    *not(struct expr * e, struct context * ctx);
struct expr    *exec(struct expr * e, struct context * ctx);

struct expr    *do_exec(struct args * a);
struct expr    *search_context(const struct context * ctx, const char *k);
struct expr    *list_add(struct expr * e, const struct expr * ne);
struct expr    *add_to_context(struct context * ctx, const char *k, const struct expr * e);
struct list    *get_list_el(const struct expr * e, int i);

int             build_argv(struct expr * e, struct args * a);
int             search_context_i(const struct context * ctx, const char *key);
int             list_len(const struct expr * e);
int             are_lists_equal(const struct list * a, const struct list * b, struct context * ctx);
int             are_exprs_equal(struct expr * a, struct expr * b, struct context * ctx, int do_eval);
int             is_valid_p_expr(const struct expr * e);
int             is_valid_lambda_expr(const struct expr * e);
int             is_function_call_expr(const struct expr * e);


#endif
