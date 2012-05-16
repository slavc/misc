/*
 * findhost.c
 *
 * The "client" counterpart to echohostnamed.c.
 *
 * If hostname is given -- scans the given IP range to find that
 * host. Otherwise prints out all hosts found in that range.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <err.h>

#define DEFAULT_PORT	1234
#define DEFAULT_TIMEOUT	100	/* in milliseconds */
#define MAKE_IP(a, b, c, d)	(((((((a & 0xff) << 8) | b & 0xff) << 8) | c & 0xff) << 8) | d & 0xff)

struct range {
	int start;
	int end;
};

void	 usage(const char *);
int	 dbgprintf(const char *, ...);
int	 read_ip(char *, struct range *);
int	 read_range(const char *, struct range *);
void	 find_host(struct range *, const char *, int, int);
char	*get_remote_hostname(int, int, int, char *, const size_t);
void	 set_nonblock_mode(int, int);

int	 dflag;

int
main(int argc, char **argv)
{
	int		 c;
	struct range	 ip[4];
	const char	*hostname = NULL;
	int		 port = DEFAULT_PORT;
	int		 timeout = DEFAULT_TIMEOUT;
	const char	*progname;
	extern char	*optarg;
	extern int	 optind;

	progname = strrchr(argv[0], '/');
	if (progname != NULL)
		++progname;
	else
		progname = argv[0];

	while ((c = getopt(argc, argv, "hdp:t:")) != -1) {
		switch (c) {
		case 'h':
			usage(progname);
			return 0;
			break;
		case 'd':
			++dflag;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			usage(progname);
			return 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc <= 0) {
		usage(progname);
		return 1;
	}

	if (read_ip(argv[0], ip) != 0) {
		usage(progname);
		return 1;
	}

	if (argc > 1)
		hostname = argv[1];

	find_host(ip, hostname, port, timeout);

	return 0;
}

void
usage(const char *progname)
{
	printf("usage: %s [-d] [-p <port>] [-t <timeout>] <ip_range> [hostname]\n", progname);
	printf("  If hostname is supplied, print on which IP it resides.\n");
	printf("  Otherwise print all hosts found in the range.\n");
}

int
dbgprintf(const char *fmt, ...)
{
	va_list ap;
	int n;

	if (dflag == 0)
		return 0;

	va_start(ap, fmt);
	n = vprintf(fmt, ap);
	va_end(ap);
	
	return n;
}

int
read_ip(char *s, struct range ip[4])
{
	char	*tok;
	int	 i;

	for (i = 0; i < 4; ++i) {
		ip[i].start = 1;
		ip[i].end = 254;
	}

	for (tok = strtok(s, "."), i = 0; tok != NULL && i < 4; tok = strtok(NULL, "."), ++i)
		if (read_range(tok, &ip[i]) != 0)
			return -1;

	return 0;
}

int
read_range(const char *s, struct range *rp)
{
	if (!strcmp(s, "*")) {
		rp->start = 1;
		rp->end = 254;
		return 0;
	} else if (sscanf(s, "%d-%d", &rp->start, &rp->end) == 2) {
		return 0;
	} else if (sscanf(s, "%d", &rp->start) == 1) {
		rp->end = rp->start;
		return 0;
	} else {
		return -1;
	}
}

void
find_host(struct range ip[4], const char *hostname, int port, int timeout)
{
	int	 a, b, c, d;
	char	 buf[256];
	int	 i;

	dbgprintf("performing search on ip range ");
	for (i = 0; i < 4; ++i) {
		if (i > 0)
			dbgprintf(".");
		dbgprintf("%d-%d", ip[i].start, ip[i].end);
	}
	dbgprintf("\n");


	for (a = ip[0].start; a <= ip[0].end; ++a) {
		for (b = ip[1].start; b <= ip[1].end; ++b) {
			for (c = ip[2].start; c <= ip[2].end; ++c) {
				for (d = ip[3].start; d <= ip[3].end; ++d) {
					dbgprintf("scanning %d.%d.%d.%d\n", a, b, c, d);
					if (get_remote_hostname(MAKE_IP(a, b, c, d), port, timeout, buf, sizeof(buf)) == NULL) {
						dbgprintf("failed to get hostname\n");
						continue;
					}

					if (hostname == NULL)
						printf("%d.%d.%d.%d %s\n", a, b, c, d, buf);
					else if (!strcmp(buf, hostname)) {
						printf("%d.%d.%d.%d %s\n", a, b, c, d, buf);
						exit(0);
					}
				}
			}
		}
	}
}

char *
get_remote_hostname(int ip, int port, int timeout, char *hostname, const size_t hostnamesize)
{
	int			 s;
	struct sockaddr_in	 sa;
        long                     flags;
	ssize_t			 nbytes;
	fd_set			 fds[1];
	struct timeval		 tv;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");

	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short) port);
	sa.sin_addr.s_addr = htonl(ip);

	dbgprintf("setting non-blocking mode on socket\n");
	set_nonblock_mode(s, 1);

	dbgprintf("calling connect...\n");
	connect(s, (struct sockaddr *) &sa, sizeof(sa));

	dbgprintf("select(2)...\n");
	FD_ZERO(fds);
	FD_SET(s, fds);
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	if (select(sizeof(fds)/sizeof(*fds), fds, fds, fds, &tv) != 0)
		err(1, "select");
	dbgprintf("select(2) returned\n");

	dbgprintf("reading the hostname... ");
	nbytes = read(s, hostname, hostnamesize - 1);
	close(s);
	if (nbytes == -1) {
		dbgprintf("read(2) failed, the socket wasn't successfully connected\n");
		return NULL;
	} else {
		hostname[nbytes] = '\0';
		return hostname;
	}
}

void
set_nonblock_mode(int fd, int value)
{
	long	 flags;

	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		err(1, "fcntl");

	if (value) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	if (fcntl(fd, F_SETFL, flags) != 0)
		err(1, "fcntl");
}
