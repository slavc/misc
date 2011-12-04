#include "lisp.h"
#include "parse.h"
#include "util.h"








int             dflag;




char           *
next_code_chunk(const char *s)
{
	if (!s)
		return NULL;

	++s;
	while (*s != '\n' && *s != '\0')
		++s;

	return (char *) s;
}


char           *
find_comment(const char *s)
{
	if (!s)
		return NULL;

	while (*s != ';' && *s != '\0')
		++s;

	if (*s == ';')
		return (char *) s;
	else
		return NULL;
}





char           *
remove_comments(char *buffer)
{
	char           *buf = NULL;
	size_t          bufsize = 0;
	size_t          len = 0;
	char           *p, *s, *e;

	if (!buffer)
		return NULL;

	for (s = buffer; s != NULL && *s != '\0'; s = next_code_chunk(e)) {
		if (e = find_comment(s))
			*e = '\0';
		if ((len = strlen(s)) == 0)
			continue;
		buf = realloc(buf, bufsize + len + (bufsize ? 1 : 2));
		memcpy(buf + (bufsize ? bufsize - 1 : 0), s, len);
		bufsize += len + (bufsize ? 1 : 2);
		buf[bufsize - 2] = ' ';
		buf[bufsize - 1] = '\0';
	}

	if (!bufsize) {
		buf = malloc(sizeof(char));
		*buf = '\0';
	} else {
		buf[bufsize - 2] = '\0';
	}

	return buf;
}











int
main(int argc, char **argv)
{
	char           *buf1, *buf2;

	if (argc < 2)
		return 0;

	buf1 = load_file(argv[1]);
	printf("----- %s -----\n\"%s\"\n", argv[1], buf1);
	buf2 = remove_comments(buf1);
	printf("----- after remove comments -----\n\"%s\"\n", buf2);

	return 0;
}
