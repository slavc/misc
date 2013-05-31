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

/**
 * Load a file into memory.
 *
 * @param fd file descriptor.
 *
 * @return malloc()ed buffer with file's content.
 */
char    *f_load(int fd);

/**
 * Read a line from file.
 *
 * @param fp file to read from.
 *
 * @return dynamically allocated buffer with the line or NULL if EOF. Newline character is retained.
 */
char	*fp_gets(FILE *fp);

#endif