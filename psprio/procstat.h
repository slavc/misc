#ifndef PROCSTAT_H
#define PROCSTAT_H

#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

struct procstat {
	long long	 pid;
	char		 name[32];
	char		 state;
	long long	 ppid;
	long long	 pgrp;
	long long	 sid;
	long long	 tty_nr;
	long long	 tty_pgrp;
	long long	 flags;
	long long	 min_flt;
	long long	 cmin_flt;
	long long	 maj_flt;
	long long	 cmaj_flt;
	long long	 utime;
	long long	 stime;
	long long	 cutime;
	long long	 cstime;
	long long	 priority;
	long long	 nice;
	long long	 num_threads;
	long long	 it_real_value;
	long long	 start_time;
	long long	 vsize;
	long long	 rss;
	long long	 rsslim;
	long long	 start_code;
	long long	 end_code;
	long long	 start_stack;
	long long	 esp;
	long long	 eip;
	long long	 pending;
	long long	 blocked;
	long long	 sigign;
	long long	 sigcatch;
	long long	 wchan;
	long long	 exit_signal;
	long long	 task_cpu;
	long long	 rt_priority;
	long long	 policy;
	long long	 blkio_ticks;
	long long	 gtime;
	long long	 cgtime;
};

bool	 procstat_parse(const char *, struct procstat *);
bool	 procstat_get(pid_t, struct procstat *);

#endif

