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

#ifndef MEM_H
#define MEM_H

#include "lisp.h"
#include "eval.h"
#include "util.h"

struct expr    *new_expr(int t);
struct list    *new_list(void);
struct atom    *new_atom(const char *s);
struct context *new_context(void);
struct list    *list_dup(const struct list * l);
struct atom    *atom_dup(const struct atom * a);
struct expr    *exprs_dup(const struct expr * e);
void           *free_atom(struct atom * a);
void           *free_list(struct list * l);
void           *free_expr(struct expr * expression);
void           *full_free_expr(struct expr * e);
void           *free_context(struct context * ctx);
void           *free_context_r(struct context * ctx);
void            free_args(struct args * a);

#endif
