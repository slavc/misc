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
#include "eval.h"
#include "mem.h"
#include "util.h"

int             dflag;

void
usage(void)
{
	printf("usage: %s [-h | -e <lisp_expr> | <filename>]\n", PROGNAME);
}

void
proc_buffer(const char *buffer)
{
	struct context *ctx;
	struct expr    *e, *er;
	const char     *p;
	char           *buf;

	ctx = new_context();
	p = skip_space(buffer);
	while (*p != '\0') {
		dbgprintf("------------------------------------------------\n");
		buf = expr_dup(p);
		dbgprintf("%s\n\n", buf);

		e = parse_expr(buf);
		ctx->is_root_level = 1;
		er = eval(e, ctx);

		dbgprintf("\n");
		if (dflag)
			peval(er, "### RESULT: ");
		print_expr(er, 0);
		printf("\n");
		dbgprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");

		Free(buf);
		free_expr(er);

		p = next_expr(p);
	}
	free_context_r(ctx);
}

int
main(int argc, char **argv)
{
	int             i;
	char           *buf, *p;

	if (argc < 2) {
		usage();
		exit(0);
	}
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h")) {
			usage();
			exit(0);
		}
	}

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-d")) {
			dflag = 1;
		} else if (!strcmp(argv[i], "-e")) {
			if (++i >= argc)
				exit(0);
			buf = strdup(argv[i]);
			proc_buffer(buf);
			Free(buf);
		} else {
			if ((buf = load_lisp_source_file(argv[i])) == NULL)
				exit(0);
			dbgprintf("--- processing buffer ---\n\"%s\"\n", buf);
			proc_buffer(buf);
			Free(buf);
		}
	}

	exit(0);
}
