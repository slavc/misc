/*
 * This code is in public domain.
 *
 * unixtime
 *
 * Print UNIX time (number of seconds since 1970-01-01) in
 * human readable form.
 */

#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int	 usage(void);
static void	 chop_newline(char *);
static int	 str2time(const char *, time_t *, int);
static void	 unixtime(time_t);

int
main(int argc, char **argv)
{
	extern int	 optind;
	int		 ch;
	time_t		 t;
	char		 line[1024];
	int		 num_base = 10;
	int		 err;

	while ((ch = getopt(argc, argv, "hx")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			return 0;
		case 'x':
			num_base = 16;
			break;
		default:
			usage();
			return -1;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		while (fgets(line, sizeof line, stdin) != NULL) {
			if (*line == '\n')
				continue;
			chop_newline(line);
			if (str2time(line, &t, num_base))
				unixtime(t);
			else
				warnx("%s: failed to convert number", line);
		}
	} else {
		while (argc > 0) {
			if (str2time(*argv, &t, num_base))
				unixtime(t);
			else
				warnx("%s: failed to convert number", *argv);
			--argc;
			++argv;
		}
	}

	return 0;
}

static int
usage(void)
{
	printf("usage: unixtime [-x] [<number>]\n\n");
	printf("Print UNIX time (seconds since the start of epoch: 1970-01-01 00:00:00)\n");
	printf("in a human readable form.\n");
	printf("If no arguments are given -- the numbers are read from stdin.\n\n");
	printf("  -h -- display this message.\n");
	printf("  -x -- convert a hexadecimal number.\n");
}

static void
chop_newline(char *s)
{
	char	*p;

	p = strchr(s, '\0') - 1;
	while (*p == '\n')
		*p-- = '\0';
}

static int
str2time(const char *s, time_t *pt, int base)
{
	unsigned long long	 ul;
	char			*endp;

	ul = strtoull(s, &endp, base);
	if (!isspace(*endp) && *endp != '\0')
		return 0;
	if (endp == s)
		return 0;
	*pt = ul;
	return 1;
}

static void
unixtime(time_t t)
{
	static char buf[] = "YYYY-MM-DD HH:MI:SS";

	strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", gmtime(&t));
	printf("%s\n", buf);
}

/* vim: noexpandtab:sts=8:shiftwidth=8:tabstop=8:cindent:cinoptions=\:0
*/
