/*
 * Display the list of processes along with their
 *  + priority
 *  + nice
 *  + real-time priority
 *  + scheduling policy
 */

#include <linux/sched.h>
#include <sched.h>
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

#include "procstat.h"

#define PROC_PATH	"/proc"

static void	 psprio(void);
static bool	 isstrdigit(const char *);

int
main(int argc, char **argv)
{
	psprio();
	return EXIT_SUCCESS;
}

static bool
isstrdigit(const char *s)
{
	while (isdigit(*s))
		++s;
	return *s == '\0';
}

static const char *
policytostr(int policy)
{
	static char	 buf[16];

#define Y(a)			\
	case a:			\
		return #a
	switch (policy) {
	Y(SCHED_OTHER);
	Y(SCHED_BATCH);
	Y(SCHED_IDLE);
	Y(SCHED_FIFO);
	Y(SCHED_RR);
	}
#undef Y

	(void) snprintf(buf, sizeof buf, "%d", policy);
	return buf;
}

static void
psprio(void)
{
	DIR		*dp;
	struct dirent	*de;
	pid_t		 pid;
	pid_t		 mypid;
	struct procstat	 ps;

	dp = opendir(PROC_PATH);
	if (dp == NULL)
		err(EXIT_FAILURE, "opendir %s", PROC_PATH);

	mypid = getpid();

	while ((de = readdir(dp)) != NULL) {
		if (!isstrdigit(de->d_name))
			continue;
		pid = atol(de->d_name);
		if (pid == mypid)
			continue;
		if (!procstat_get(pid, &ps))
			warnx("procstat_get %lu", (unsigned long) pid);
		printf("%lu\t%32s\t%lld\t%lld\t%lld\t%s\n",
		    (unsigned long) ps.pid,
		    ps.name,
		    ps.priority,
		    ps.nice,
		    ps.rt_priority,
		    policytostr(ps.policy));
	}

	if (closedir(dp) < 0)
		warn("closedir");
}
