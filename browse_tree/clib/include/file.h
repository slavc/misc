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

#ifndef CLIB_FILE_H
#define CLIB_FILE_H

#include <stdio.h>

enum file_types {
	FILE_TYPE_NORMAL,
	FILE_TYPE_GZ,
};

struct f_file;
typedef struct f_file *f_file_t;
struct f_file {
	enum file_types	 type;
	union {
		FILE	*fp;
		gzFile	 fgz;
	} f;
};

f_file_t	 f_open(const char *path, const char *mode);
ssize_t		 f_read(f_file_t f, char *buf, ssize_t len);
ssize_t		 f_write(f_file_t f, const char *buf, ssize_t len);
char		*f_gets(f_file_t f);
void		 f_close(f_file_t f);

#endif
