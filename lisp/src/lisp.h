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

#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define PROGNAME "lisp"

struct expr {
	int             t;
	int             quoted;
	union {
		struct atom {
			char           *v;
		}              *atom;
		struct list {
			struct expr    *v;
			struct list    *next;
		}              *list;
	}               v;
#define vatom v.atom
#define vlist v.list
};

enum expr_types {
	LINVALID = -1,
	LATOM,
	LLIST
};

struct context {
	int             nmap;
	struct map {
		char           *k;
		struct expr    *v;
	}              *map;
	struct context *next;

	int             is_root_level;
};

#endif
