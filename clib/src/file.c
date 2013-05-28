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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <xmem.h>

char *
f_load(int fd)
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

char *
fp_gets(FILE *fp)
{
	/* FIXME
	 * Examine popular libc implementations to determine 
	 * the optimal value for chunksize.
	 */
	const size_t	 chunksize = 512;
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

#ifdef TEST_F_LOAD
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	int	 fd;
	char	*buf;

	while (++argv, --argc) {
		fd = open(*argv, O_RDONLY);
		if (fd == -1) {
			warn("%s", *argv);
			continue;
		}
		buf = f_load(fd);
		close(fd);
		printf("%s:\n\"%s\"\n", *argv, buf);
                xfree(buf);
	}

        return 0;
}
#endif

#ifdef TEST_FP_GETS
#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
{
	FILE	*fp;
	char	*buf;

	while (++argv, --argc) {
		fp = fopen(*argv, "r");
		if (fp == NULL) {
			warn("%s", *argv);
			continue;
		}
		while ((buf = fp_gets(fp)) != NULL) {
			printf("%s", buf);
			xfree(buf);
		}
		fclose(fp);
	}

        return 0;
}
#endif
