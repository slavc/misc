#ifndef PROCSTAT_H
#define PROCSTAT_H

#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

struct procstat {
	unsigned long long	 pid;
	char			 name[32];
	char			 state;
	unsigned long long	 ppid;
	unsigned long long	 pgrp;
	unsigned long long	 sid;
	unsigned long long	 tty_nr;
	unsigned long long	 tty_pgrp;
	unsigned long long	 flags;
	unsigned long long	 min_flt;
	unsigned long long	 cmin_flt;
	unsigned long long	 maj_flt;
	unsigned long long	 cmaj_flt;
	unsigned long long	 utime;
	unsigned long long	 stime;
	unsigned long long	 cutime;
	unsigned long long	 cstime;
	unsigned long long	 priority;
	unsigned long long	 nice;
	unsigned long long	 num_threads;
	unsigned long long	 it_real_value;
	unsigned long long	 start_time;
	unsigned long long	 vsize;
	unsigned long long	 rss;
	unsigned long long	 rsslim;
	unsigned long long	 start_code;
	unsigned long long	 end_code;
	unsigned long long	 start_stack;
	unsigned long long	 esp;
	unsigned long long	 eip;
	unsigned long long	 pending;
	unsigned long long	 blocked;
	unsigned long long	 sigign;
	unsigned long long	 sigcatch;
	unsigned long long	 wchan;
	unsigned long long	 exit_signal;
	unsigned long long	 task_cpu;
	unsigned long long	 rt_priority;
	unsigned long long	 policy;
	unsigned long long	 blkio_ticks;
	unsigned long long	 gtime;
	unsigned long long	 cgtime;
};

bool	 procstat_parse(const char *, struct procstat *);
bool	 procstat_get(pid_t, struct procstat *);

#endif

