/*
 * This code is in public domain.
 *
 * unixtime
 *
 * Convert the number of seconds since the beginning
 * of epoch into human readable form.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

static int is_dec_number(const char *);

int
main(int argc, char **argv)
{
	time_t t;
	char buf[] = "YYYY-MM-DD HH:MI:SS";

	while (++argv, --argc) {
		t = strtoull(*argv, NULL, is_dec_number(*argv) ? 10 : 16);
		strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", gmtime(&t));
		printf("%s\n", buf);
	}

	return 0;
}

static int
is_dec_number(const char *s)
{
	while (*s != '\0')
		if (!isdigit(*s++))
			return 0;
	return 1;
}
