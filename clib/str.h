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

#ifndef STR_H
#define STR_H

/**
 * Prepend `s' with a string formatted from `fmt' and va_args.
 *
 * @parameter s pointer to a dynamically allocated string or NULL.
 * @parameter fmt format for the va arguments.
 *
 * @return pointer to reallocated `s'.
 */
char	*str_prepend(char *s, const char *fmt, ...);

/**
 * Return non-zero if string only consists of isspace(3) chars.
 */
int	 str_is_space(const char *s);

#endif
