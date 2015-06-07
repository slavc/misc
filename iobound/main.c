/*
 * Copyright (c) 2015 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

/*
 * List processes and tell whether they are CPU or I/O bound, based on the number
 * of voluntary context switches versus involuntary context switches.
 *
 * An I/O bound process will spend most of it's time sleeping and thus should
 * have more voluntary switches compared to involuntary switches.
 * And vice-versa, a CPU bound process should have more involuntary context
 * switches compared to voluntary context switches.
 *
 * This program is probably very imprecise.
 *
 * Output format:
 * <pid> <bound> <name>
 *  <pid>   -- process identifier;
 *  <bound> -- "cpu", "io", "both" if the program appears to be bound by both,
 *             "n/a" if there isn't enough information;
 *  <name>  -- name of the program.
 */

#include <sys/types.h>
#include <unistd.h>
#include <err.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#define PROCFS_PATH	"/proc"
#define TOTAL_THRESHOLD	30
#define DIFF_THRESHOLD	0.25

enum bound {
	NA,
	BOTH,
	CPU,
	IO
};
static const char *boundstr[] = {
	"n/a",
	"both",
	"cpu",
	"io"
};

struct procinfo {
	pid_t		 pid;
	char		 name[32];
	unsigned long	 volun;		// number of voluntary context switches
	unsigned long	 involun;	// number of involuntary context switches
};

static bool	 isstrnumeric(const char *);
static bool	 startswith(const char *, const char *);
static bool	 readprocinfo(pid_t, struct procinfo *);
static void	 iobound(void);

int
main(int argc, char **argv)
{
	iobound();

	return EXIT_SUCCESS;
}

static bool
isstrnumeric(const char *s)
{
	while (isdigit(*s))
		++s;
	return *s == '\0';
}

static bool
startswith(const char *prefix, const char *s)
{
	while (*prefix == *s) {
		++prefix;
		++s;
	}
	return *prefix == '\0';
}

// Remove newline char from the end of a string.
static void
zapnl(char *s)
{
	s = strrchr(s, '\n');
	if (s != NULL)
		*s = '\0';
}

static bool
readprocinfo(pid_t pid, struct procinfo *pi)
{
	char	 path[128];
	int	 n;

	n = snprintf(path, sizeof path, PROCFS_PATH "/%ld/status", (long) pid);
	if (n >= sizeof path)
		err(EXIT_FAILURE, "snprintf output truncated");

	FILE		*fp;
	char		 line[128];
	char		*p;
	const char	 volun[] = "voluntary_ctxt_switches";
	const char	 involun[] = "nonvoluntary_ctxt_switches";
	const char	 name[] = "Name";
	void		*valp;
	bool		 retval = true;

	memset(pi, 0, sizeof *pi);

	pi->pid = pid;

	fp = fopen(path, "r");
	if (fp == NULL) {
		warn("fopen %s", path);
		return false;
	}

	while (fgets(line, sizeof line, fp) != NULL) {
		if (startswith(volun, line))
			valp = &pi->volun;
		else if (startswith(involun, line))
			valp = &pi->involun;
		else if (startswith(name, line))
			valp = &pi->name;
		else
			continue;
		p = strrchr(line, '\t');
		if (p++ == NULL) {
			warnx("%s: invalid format", path);
			retval = false;
			goto out;
		}
		if (valp == &pi->name) {
			zapnl(line);
			snprintf(pi->name, sizeof pi->name, "%s", p);
		} else { // volun or involun
			*((unsigned long *) valp) = strtoul(p, NULL, 10);
			if (*((unsigned long *) valp) == ULONG_MAX
			    && errno == ERANGE) {
				warnx("%s: integer overflow", path);
				retval = false;
				goto out;
			}
		}
	}
out:
	fclose(fp);
	return retval;
}

static enum bound
computebound(struct procinfo *pi)
{
	double		 volun;
	double		 involun;
	double		 total;

	volun = pi->volun;
	involun = pi->involun;
	total = volun + involun;
	if (total < TOTAL_THRESHOLD)
		return NA;

	volun = volun / total;
	involun = involun / total;
	if (fabs(volun - involun) > DIFF_THRESHOLD) {
		if (volun > involun)
			return IO;
		else
			return CPU;
	} else
		return BOTH;
}

static void
iobound(void)
{
	DIR		*dp;
	struct dirent	*de;
	pid_t		 pid;
	struct procinfo	 pi;
	enum bound	 bound;

	dp = opendir(PROCFS_PATH);
	if (dp == NULL)
		err(EXIT_FAILURE, "opendir %s", PROCFS_PATH);

	while ((de = readdir(dp)) != NULL) {
		if (!isstrnumeric(de->d_name))
			continue;
		pid = atol(de->d_name);
		if (pid == getpid())
			continue;
		if (readprocinfo(pid, &pi)) {
			bound = computebound(&pi);
			printf("%ld\t%s\t%s\t\n", (long) pi.pid, boundstr[bound], pi.name);
		}
	}
}
