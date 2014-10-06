#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "procstat.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))

bool
procstat_parse(const char *s, struct procstat *ps)
{
	void	*fields[] = {
		&ps->pid,
		&ps->name,
		&ps->state,
		&ps->ppid,
		&ps->pgrp,
		&ps->sid,
		&ps->tty_nr,
		&ps->tty_pgrp,
		&ps->flags,
		&ps->min_flt,
		&ps->cmin_flt,
		&ps->maj_flt,
		&ps->cmaj_flt,
		&ps->utime,
		&ps->stime,
		&ps->cutime,
		&ps->cstime,
		&ps->priority,
		&ps->nice,
		&ps->num_threads,
		&ps->it_real_value,
		&ps->start_time,
		&ps->vsize,
		&ps->rss,
		&ps->rsslim,
		&ps->start_code,
		&ps->end_code,
		&ps->start_stack,
		&ps->esp,
		&ps->eip,
		&ps->pending,
		&ps->blocked,
		&ps->sigign,
		&ps->sigcatch,
		&ps->wchan,
		NULL, // deprecated fields
		NULL,
		&ps->exit_signal,
		&ps->task_cpu,
		&ps->rt_priority,
		&ps->policy,
		&ps->blkio_ticks,
		&ps->gtime,
		&ps->cgtime,
	},			**fieldp = fields;
	size_t			 nfields = sizeof fields / sizeof *fields;
	const char		*p = s;
	char			*endptr;
	size_t			 len;
	unsigned long long	 val;

	while (nfields--) {
		if (*fieldp == &ps->name) {
			s = strchr(s, '(');
			p = strrchr(s, ')');
			if (s == NULL || p == NULL)
				return false;
			len = MIN(p - s - 1, sizeof(ps->name) - 1);
			memcpy(ps->name, s + 1, len);
			ps->name[len] = '\0';
			s = p + 1;
		} else if (*fieldp == &ps->state) {
			*((char *) *fieldp) = *++s;
			++s;
		} else {
			errno = 0;
			val = strtoull(s, &endptr, 10);
			s = endptr;
			if (val == LLONG_MAX && errno == ERANGE)
				return false;
			if (val == 0 && errno == EINVAL)
				return false;
			if (*fieldp != NULL)
				*((unsigned long long *) *fieldp) = (long long) val;
		}
		++fieldp;
	}

	return true;
}

bool
procstat_get(pid_t pid, struct procstat *ps)
{
	char	 path[64];

	if (snprintf(path, sizeof path, "/proc/%lu/stat", (unsigned long) pid) >= sizeof path)
		return false;


	int	 fd;
	char	 buf[1024];
	ssize_t	 n;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return false;
	n = read(fd, buf, sizeof(buf) - 1);
	(void) close(fd);
	if (n <= 0)
		return false;
	buf[n] = '\0';

	return procstat_parse(buf, ps);
}

