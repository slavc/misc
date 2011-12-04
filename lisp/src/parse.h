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

#ifndef PARSE_H
#define PARSE_H

#include "lisp.h"
#include "util.h"

char           *find_space(const char *s);
char           *skip_space(const char *s);
char           *find_matching_parenthesis(const char *s);
char           *find_expr_end(const char *e);
char           *next_expr(const char *e);
char           *expr_dup(const char *e);
int             is_atom(const char *e);
int             is_list(const char *e);
struct list    *parse_list(const char *e);
struct atom    *parse_atom(const char *e);
struct expr    *parse_expr(const char *e);
char           *next_code_chunk(const char *s);
char           *find_comment(const char *s);
char           *remove_comments(char *buffer);
char           *load_lisp_source_file(const char *filename);

#endif
