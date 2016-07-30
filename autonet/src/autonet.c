#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <signal.h>
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SCRIPTS_PATH	"/etc/autonet"
#define POLL_INTERVAL	2 // seconds

bool is_in_array(char *ifname, char **ifnames);
bool is_in_list(char *ifname, struct ifaddrs *iflist);
char **append_name(char *ifname, char **ifnames);
char **remove_name(char *ifname, char **ifnames);
size_t array_len(char **ifnames);
void run_script(const char *name, const char *ifname);
void ignore_signal(int signo);

int
main(int argc, char **argv)
{
	char **ifnames = NULL;
	char **ifname;
	struct ifaddrs *iflist = NULL;
	struct ifaddrs *ifp;

	signal(SIGHUP, SIG_IGN);

	for (;;) {
		if (getifaddrs(&iflist) == -1)
			err(1, "getifaddrs");
		for (ifp = iflist; ifp != NULL; ifp = ifp->ifa_next) {
			if (!is_in_array(ifp->ifa_name, ifnames)) {
				run_script("onattach", ifp->ifa_name);
				ifnames = append_name(ifp->ifa_name, ifnames);
			}
		}
restart:
		for (ifname = ifnames; ifname != NULL && *ifname != NULL; ifname++) {
			if (!is_in_list(*ifname, iflist)) {
				run_script("ondetach", *ifname);
				ifnames = remove_name(*ifname, ifnames);
				goto restart;
			}
		}
		freeifaddrs(iflist);
		sleep(POLL_INTERVAL);
	}

	return 0;
}

bool
is_in_array(char *ifname, char **ifnames)
{
	for (char **p = ifnames; p != NULL && *p != NULL; p++)
		if (!strcmp(ifname, *p))
			return true;
	return false;
}

bool
is_in_list(char *ifname, struct ifaddrs *ifp)
{
	while (ifp != NULL) {
		if (!strcmp(ifname, ifp->ifa_name))
			return true;
		ifp = ifp->ifa_next;
	}
	return false;
}

char **
append_name(char *ifname, char **ifnames)
{
	char **p;
	int len = 1;

	for (p = ifnames; p != NULL && *p != NULL; p++)
		len++;

	p = reallocarray(ifnames, len + 1, sizeof(char *));
	if (p == NULL)
		err(1, "reallocarray");
	ifname = strdup(ifname);
	if (ifname == NULL)
		err(1, "strdup");
	p[len - 1] = ifname;
	p[len] = NULL;

	return p;
}

char **
remove_name(char *ifname, char **ifnames)
{
	size_t i;
	size_t len;

	if (ifnames == NULL)
		return NULL;

	len = array_len(ifnames);

	for (i = 0; ifnames[i] != NULL; i++) {
		if (!strcmp(ifname, ifnames[i])) {
			free(ifnames[i]);
			memmove(ifnames + i, ifnames + i + 1, (len - i) * sizeof(char *));
			ifnames = reallocarray(ifnames, len, sizeof(char *));
			if (ifnames == NULL)
				err(1, "reallocarray");
			break;
		}
	}
	return ifnames;
}

size_t
array_len(char **ifnames)
{
	char **ifname;
	size_t len = 0;

	for (ifname = ifnames; ifname != NULL && *ifname != NULL; ifname++)
		len++;
	return len;
}

void
run_script(const char *name, const char *ifname)
{
	size_t len = strlen(SCRIPTS_PATH) + 1 + strlen(name);
	char path[len + 1];
	pid_t pid;

	(void)snprintf(path, sizeof path, "%s/%s", SCRIPTS_PATH, name);

	pid = fork();

	if (pid == -1)
		err(1, "fork");
	if (pid > 0) {
		if (wait(NULL) == -1)
			err(1, "wait");
		return;
	}

	if (execl(path, path, ifname, NULL) == -1)
		err(1, "execl");
}
