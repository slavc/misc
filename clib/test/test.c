#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include <str.h>
#include <xmem.h>
#include <file.h>
#include <util.h>

#define ASSERT(expr) \
	do { \
		if (!expr) \
			errx(1, "unit tests failed: line %d, expression is false: %s\n", __LINE__, #expr); \
	} while (0)

static void test_str(void);

int
main(int argc, char **argv)
{
	test_str();
	return 0;
}

static void
test_str(void)
{
	char	*s;

	s = NULL;
	s = str_prepend(s, "%s", "sys");
	s = str_prepend(s, "%s.", "ctl");
	s = str_prepend(s, "%s.", "abc");

	ASSERT(strcmp(s, "abc.ctl.sys") == 0);

	xfree(s);
	s = NULL;
	s = str_append(s, "aaa");
	s = str_append(s, "%d", 42);
	s = str_append(s, "bbbb");
	s = str_append(s, "XXXXXXXX%d", 999);

	ASSERT(strcmp(s, "aaa42bbbbXXXXXXXX999") == 0);
}
