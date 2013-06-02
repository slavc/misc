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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <zlib.h>

#include <file.h>
#include <xmem.h>
#include <str.h>

#define READ_BUF_SIZE	4096
#define INVALID_FILE_ARGUMENT() \
	do { \
		fprintf(stderr, "%s(): invalid file argument", __func__); \
		abort(); \
	} while(0)
#define DBG(fmt, ...)

static char	*fgz_gets(gzFile f);
static char	*fp_gets(FILE *fp);

f_file_t
f_open(const char *path, const char *mode)
{
	f_file_t	 f = NULL;
	const char	*p;

	f = xmalloc(sizeof(*f));
	p = strrchr(path, '.');
	if (p != NULL && !strcasecmp(p + 1, "gz")) {
		f->type = FILE_TYPE_GZ;
		f->f.fgz = gzopen(path, mode);
		if (f->f.fgz == NULL)
			goto fail;
	} else {
		f->type = FILE_TYPE_NORMAL;
		f->f.fp = fopen(path, mode);
		if (f->f.fp == NULL)
			goto fail;
	}

	return f;

fail:
	xfree(f);
	return NULL;
}

ssize_t
f_read(f_file_t f, char *buf, ssize_t len)
{
	if (f->type == FILE_TYPE_NORMAL)
		return fread(buf, sizeof(*buf), len, f->f.fp);
	else if (f->type == FILE_TYPE_GZ)
		return gzread(f->f.fgz, buf, len);
	INVALID_FILE_ARGUMENT();
}

ssize_t
f_write(f_file_t f, const char *buf, ssize_t len)
{
	if (f->type == FILE_TYPE_NORMAL)
		return fwrite(buf, len, sizeof(*buf), f->f.fp);
	else if (f->type == FILE_TYPE_GZ)
		return gzwrite(f->f.fgz, buf, len);
	INVALID_FILE_ARGUMENT();
}

char *
f_gets(f_file_t f)
{
	if (f->type == FILE_TYPE_NORMAL)
		return fp_gets(f->f.fp);
	else if (f->type == FILE_TYPE_GZ)
		return fgz_gets(f->f.fgz);
	INVALID_FILE_ARGUMENT();
}

void
f_close(f_file_t f)
{
	if (f->type == FILE_TYPE_NORMAL)
		fclose(f->f.fp);
	else if (f->type == FILE_TYPE_GZ)
		gzclose(f->f.fgz);
        else {
            INVALID_FILE_ARGUMENT();
        }
}

static char *
fgz_gets(gzFile f)
{
	static char		 buf[READ_BUF_SIZE];
	const size_t		 bufsize = sizeof buf;
	static char		*nl = NULL;
	char			*next_nl;
	int			 nread;
	char			*line = NULL;

	if (nl != NULL) {
		next_nl = strchr(nl + 1, '\n');
		if (next_nl != NULL)
			*next_nl = '\0';
		line = str_append(line, "%s", nl + 1);
		if (next_nl != NULL) {
			nl = next_nl;
			return line;
		}
		nl = NULL;
	}
	do {
		nread = gzread(f, buf, bufsize - 1);
		if (nread <= 0)
			break;
		buf[nread] = '\0';
		DBG("buf=``%s''\n", buf);
		nl = strchr(buf, '\n');
		if (nl != NULL)
			*nl = '\0';
		line = str_append(line, "%s", buf);
		if (nl != NULL)
			break;
		if (nread < (bufsize - 1))
			break;
	} while (1);

	return line;
}

static char *
fp_gets(FILE *fp)
{
	const size_t	 chunksize = READ_BUF_SIZE;
	char		*buf = NULL;
	size_t		 bufsize;
	size_t		 i;

	i = bufsize = 0;
	for ( ;; ) {
		bufsize += chunksize;
		buf = xrealloc(buf, bufsize);
		if (fgets(buf + i, bufsize - i, fp) == NULL) {
			if (i == 0) {
				xfree(buf);
				return NULL;
			} else {
				buf = xrealloc(buf, strlen(buf) + 1);
				return buf;
			}
		}
		if (strchr(buf + i, '\n') != NULL)
			return buf;
		i = bufsize - 1;
	}
}

#if 0
char *
fd_load(int fd)
{
	char		*buf;
	size_t		 bufsize;
	const size_t	 chunksize = 8192;
	ssize_t		 nread;

	lseek(fd, 0, SEEK_CUR);

	buf = NULL;
	bufsize = 0;
	nread = 0;
	do {
		buf = xrealloc(buf, bufsize + chunksize);
		nread = read(fd, buf + bufsize, chunksize);
		bufsize += chunksize;
	} while (nread == chunksize);

	if (nread < 0)
		err(1, "read");

	bufsize = bufsize - chunksize + nread + 1;
	buf = xrealloc(buf, bufsize);
	buf[bufsize - 1] = '\0';
	return buf;
}
#endif
