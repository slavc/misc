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

/**
 * File operations.
 */

#include <stdio.h>
#include <zlib.h>

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

/**
 * Open a file.
 *
 * Opens a file and returns a file object.
 *
 * @param path path to the file.
 * @param mode access mode with which to open the file.
 *
 * @return A file object if successful, NULL otherwise.
 */
f_file_t	 f_open(const char *path, const char *mode);

/**
 * Read from a file.
 *
 * @param f file from which to read.
 * @param buf pointer to a buffer where to store the bytes read.
 * @param len how many bytes to read.
 *
 * @return Number of bytes read.
 */
ssize_t		 f_read(f_file_t f, char *buf, ssize_t len);

/**
 * Write to a file.
 *
 * @param f file to which to write.
 * @param buf pointer to a buffer which contains the data to be written.
 * @param len how many bytes to write.
 *
 * @return Number of bytes written.
 */
ssize_t		 f_write(f_file_t f, const char *buf, ssize_t len);

/**
 * Read a line from a file.
 *
 * @param f file from which to read.
 *
 * @return A dynamically allocated buffer which contains a single line from the file.
 */
char		*f_gets(f_file_t f);

/**
 * Close a file.
 *
 * @param f file which to close.
 */
void		 f_close(f_file_t f);

#endif
