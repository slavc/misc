#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <str.h>

//#define DBG(fmt, ...) printf("dbg: " fmt, __VA_ARGS__)
#define DBG(fmt, ...)

static char *
gz_read_line(gzFile f)
{
	static char		 buf[256];
	const size_t		 bufsize = sizeof buf;
	static char		*nl = NULL;
	char			*next_nl;
	int			 nread;
	char			*line = NULL;
	size_t			 linesize = 0;

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

int
main(int argc, char **argv)
{
	gzFile	 f;
	char	*line;
	int	 line_no;

	while (++argv, --argc) {
		f = gzopen(*argv, "rb");
		if (f == NULL) {
			warnx("%s: failed to open", *argv);
			continue;
		}
		line_no = 0;
		while ((line = gz_read_line(f)) != NULL) {
			printf("line %d: %s\n", ++line_no, line);
			xfree(line);
		}
		gzclose(f);
	}

	return 0;
}
