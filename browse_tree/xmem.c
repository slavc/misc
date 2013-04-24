#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xmem.h"

void *
xmalloc(size_t size)
{
	void	*p;

	p = malloc(size);
	if (p == NULL)
		err(1, "malloc");
	return p;
}

void *
xcalloc(size_t n, size_t size)
{
	void	*p;

	p = calloc(n, size);
	if (p == NULL)
		err(1, "calloc");
	return p;
}

void *
xrealloc(void *p, size_t size)
{
	void	*q;

	q = realloc(p, size);
	if (q == NULL)
		err(1, "realloc");
	return q;
}

void
xfree(void *p)
{
	free(p);
}

char *
xstrdup(const char *s)
{
	char	*p;

	p = strdup(s);
	if (s == NULL)
		err(1, "strdup");
	return p;
}
