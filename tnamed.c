/* Run it on all computers in a local network and each computer will
 * add the hostnames of it's peers into it's /etc/hostnames.
 */

/*
 * Copyright (c) 2009 Sviatoslav Chagaev <slava@zb.lv>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <err.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>
#include <arpa/inet.h>

#define PROGNAME "tnamed"

#define BUFSIZE 128
#define HOSTNAMESIZE 32
#define IP_ADDRSIZE 16
#define MAX_MAP_SIZE 255
#define DEFAULT_PORT 1392
#define COPY_SUFF ".orig"
#define DEFAULT_HOSTS_FILENAME "/etc/hosts"

#ifdef BSD
#	define IF_BSD(x) x
#else
#	define IF_BSD(x)
#endif

struct {
	int dflag;
	char *hosts_filename;
} opts;

struct hostname_mapping {
	char hostname[HOSTNAMESIZE];
	in_addr_t ip_addr;
};
struct hostname_map {
	struct hostname_mapping *map;
	int n;
	int maxn;
};

struct my_info {
	char hostname[HOSTNAMESIZE];
	in_addr_t *ip_addrs;
	int n_ip_addrs;
};

extern int errno;

int
dbgprintf(const char *fmt, ...) {
	int n;
	va_list ap;
	
	if (!opts.dflag)
		return 0;
	
	va_start(ap, fmt);
	n = vfprintf(stdout, fmt, ap);
	va_end(ap);
	
	return n;
}

u_int32_t
make_ip_addr(int a, int b, int c, int d) {
	u_int32_t l;
	l = ((a << 24) & 0xff000000) | ((b << 16) & 0xff0000) | ((c << 8) & 0xff00) | (d & 0xff);
	return htonl(l);
}

u_int32_t
str_to_ip(const char *s) {
	int a, b, c, d;
	sscanf(s, "%i.%i.%i.%i", &a, &b, &c, &d);
	return make_ip_addr(a, b, c, d);
}

char *
encode_request_msg(char buf[BUFSIZE]) {
	snprintf(buf, BUFSIZE, "R");
	return buf;
}

char *
encode_announce_msg(char buf[BUFSIZE], const char *hostname, in_addr_t ip) {
	struct in_addr in_addr;
	in_addr.s_addr = ip;
	snprintf(buf, BUFSIZE, "A%s@%s", hostname, inet_ntoa(in_addr));
	return buf;
}

void
broadcast_msg(const char *msg, const size_t msgsize, in_addr_t ip_addr) {
	int s;
	struct sockaddr_in sa;
	long i;
	in_addr_t ip;
	
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		warn("socket");
		return;
	}
	
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(DEFAULT_PORT);
	IF_BSD((sa.sin_len = sizeof(sa)));
	
	for (i = 1; i < 255; ++i) {
		ip = ntohl(ip_addr);
		ip = htonl((ip & 0xffffff00) | i);
		
		if (ip == ip_addr)
			continue;
		
		sa.sin_addr.s_addr = ip;
		if (sendto(s, msg, msgsize, 0, (struct sockaddr *) &sa, sizeof(sa)) == -1)
			warn("sendto");
	}
	close(s);
}

in_addr_t
get_iface_ip_addr(const char *iface_name) {
	struct ifreq ifreq;
	struct sockaddr_in *sa;
	int s;
	
	bzero(&ifreq, sizeof(ifreq));
	strncpy(ifreq.ifr_name, iface_name, IFNAMSIZ - 1);
	
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		err(errno, "socket");
	if (ioctl(s, SIOCGIFADDR, &ifreq) == -1)
		err(errno, "ioctl");
	close(s);
	sa = (struct sockaddr_in *) &ifreq.ifr_addr;
	
	return sa->sin_addr.s_addr;
}

char *
get_hostname(char *buf, size_t bufsize) {
	char *p;
	
	if (gethostname(buf, bufsize) == -1)
		err(errno, "gethostname");
	if (p = strchr(buf, '.'))
		*p = '\0';
	return buf;
}

int
listen_udp_port(int port) {
	int s;
	struct sockaddr_in sa;
	
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		err(errno, "socket");
	
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = INADDR_ANY;
	IF_BSD((sa.sin_len = sizeof(sa)));
	
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1)
		err(errno, "bind");
	
	return s;
}

void
hex_dump(void *buf, size_t size) {
	unsigned char *p = buf;
	size_t n;
	
	for (n = 0; n < size; ++n) {
		if (n != 0) {
			if (n % 8 == 0)
				printf("\n");
			else if (n % 4 == 0)
				printf(" ");
		}
		printf("%02x", p[n]);
	}
	printf("\n");
}

void
print_ifc(struct ifconf *ifc) {
	int i;
	void *p;
	struct ifreq *ifr;
	struct in_addr in_addr;
	size_t len = 0;
    
	dbgprintf("[ifc]\n", ifc->ifc_len);
	for (p = ifc->ifc_buf; p < (void *) ifc->ifc_buf + ifc->ifc_len; p += len) {
		ifr = (struct ifreq *) p;
		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:
			in_addr.s_addr = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;
			dbgprintf("if_name = %s; ip_addr = %s; sa_family = %d\n", ifr->ifr_name, inet_ntoa(in_addr), ifr->ifr_addr.sa_family);
			break;
		default:
			dbgprintf("unsupported address family %d\n", ifr->ifr_addr.sa_family);
			break;
		}
#ifdef BSD
		len = MAX(sizeof(struct sockaddr), ifr->ifr_addr.sa_len) + sizeof(ifr->ifr_name);
#else
		len = sizeof(struct ifreq);
#endif
	}
}

int
is_ordinary_ip(in_addr_t ip) {
	static in_addr_t lo_ip = 0;
	if (lo_ip == 0)
		lo_ip = make_ip_addr(127, 0, 0, 1);
	return ip != lo_ip && ip != INADDR_ANY && ip != INADDR_BROADCAST;
}

void
get_my_info(struct my_info *mi) {
	char hostname[HOSTNAMESIZE];
	in_addr_t ip_addr;
	struct ifconf ifc;
	int s;
	in_addr_t *ips;
	int n_ips;
	size_t ifc_buf_size = 100 * sizeof(struct ifreq);
	struct ifreq *ifr;
	void *p;
	size_t len;
	
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		err(errno, "socket");
	
	get_hostname(hostname, HOSTNAMESIZE);
	memcpy(mi->hostname, hostname, HOSTNAMESIZE);
	
	/* Get our host's IP addresses */
	bzero(&ifc, sizeof(ifc));
    ioctl(s, SIOCGIFCONF, &ifc);
	if ((ifc.ifc_buf = calloc(1, ifc.ifc_len)) == NULL)
		err(errno, "calloc");
	ioctl(s, SIOCGIFCONF, &ifc);
	if (opts.dflag)
		print_ifc(&ifc);
	for (p = ifc.ifc_buf; p < (void *) ifc.ifc_buf + ifc.ifc_len; p += len) {
		ifr = (struct ifreq *) p;
#ifdef BSD
		len = MAX(sizeof(struct sockaddr), ifr->ifr_addr.sa_len) + sizeof(ifr->ifr_name);
#else
		len = sizeof(struct ifreq);
#endif
		if (ifr->ifr_addr.sa_family != AF_INET) {
            dbgprintf("dropping record, not AF_INET\n");
			continue;
        }
		ip_addr = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;
		if (!is_ordinary_ip(ip_addr)) {
            dbgprintf("dropping record, not ordinary ip\n");
			continue;
        }
        dbgprintf("adding ip to my ips\n");
		n_ips = mi->n_ip_addrs + 1;
		if ((ips = realloc(mi->ip_addrs, sizeof(in_addr_t) * n_ips)) == NULL) {
			warn("realloc");
			break;
		}
		ips[n_ips-1] = ip_addr;
		mi->ip_addrs = ips;
		mi->n_ip_addrs = n_ips;
	}
	free(ifc.ifc_buf);
	close(s);
}

void
print_my_info(struct my_info *mi) {
	int i;
	struct in_addr in_addr;
	
	if (!mi) {
		dbgprintf("(null)\n");
		return;
	}
	
	dbgprintf("[my_info]\n");
	dbgprintf("hostname = %s\n", mi->hostname);
	for (i = 0; i < mi->n_ip_addrs; ++i) {
		in_addr.s_addr = mi->ip_addrs[i];
		dbgprintf("ip_addrs[%i] = %s\n", i, inet_ntoa(in_addr));
	}
	dbgprintf("\n");
}

void
add_mapping(struct hostname_map *m, struct hostname_mapping *mapp) {
	struct hostname_mapping *v;
	int n;
	int i;
	
	for (i = 0; i < m->n; ++i)
		if (!strcmp(m->map[i].hostname, mapp->hostname))
			break;
	if (i != m->n) {
		m->map[i].ip_addr = mapp->ip_addr;
		return;
	}
	n = m->n + 1;
	if (n >= m->maxn || m->maxn == 0)
		return;
	if ((v = realloc(m->map, sizeof(struct hostname_mapping) * n)) == NULL) {
		warn("realloc");
		return;
	}
	memcpy(v + n - 1, mapp, sizeof(struct hostname_mapping));
	m->map = v;
	m->n = n;
}

void
print_hostname_map(struct hostname_map *m) {
	int i;
	struct in_addr in_addr;
	
	dbgprintf("[hostname_map]\n");
	if (!m->n) {
		dbgprintf("(empty)\n");
		return;
	}
	for (i = 0; i < m->n; ++i) {
		in_addr.s_addr = m->map[i].ip_addr;
		dbgprintf("%s:%s\n", m->map[i].hostname, inet_ntoa(in_addr));
	}
	dbgprintf("\n");
}

void
copy_file(const char *from, const char *to) {
	enum { bufsize = 16 };
	char buf[bufsize];
	size_t n;
	FILE *fin, *fout;
	
	if ((fin = fopen(from, "r")) == NULL) {
		dbgprintf("\"%s\": %s\n", from, strerror(errno));
		return;
	}
	if ((fout = fopen(to, "w")) == NULL) {
		dbgprintf("\"%s\": %s\n", to, strerror(errno));
		fclose(fin);
		return;
	}
	
	while ((n = fread(buf, 1, bufsize, fin)) > 0)
		fwrite(buf, 1, n, fout);
	fclose(fin);
	fclose(fout);
}

/* Returns a dynamically allocated buffer */
char *
load_file(const char *filename) {
	FILE *fp;
	size_t n;
	enum { chunksize = 16 };
	char chunk[chunksize];
	char *buf = NULL;
	size_t bufsize = 0;
	char *p = NULL;
	
	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;
	while ((n = fread(chunk, 1, chunksize, fp)) > 0) {
		if ((p = realloc(buf, bufsize + n + 1)) == NULL) {	/* +1 for '\0'		*/
			fclose(fp);
			dbgprintf("realloc: %s\n", strerror(errno));
			return buf;
		}
		buf = p;
		memcpy(buf + bufsize, chunk, n);
		bufsize += n;
		buf[bufsize] = '\0';								/* no -1, see above	*/
	}
	fclose(fp);
	
	return buf;
}

void
update_hosts_file(struct hostname_map *m, const char *filename) {
	FILE *fp;
	static char *original_content = NULL;
	static int file_copied = 0;
	char *p;
	int i;
	struct in_addr in_addr;
	
	if (!original_content) {
		if ((original_content = load_file(opts.hosts_filename)) == NULL)
			original_content = "";
	}
	
	if (!file_copied) {
		if (p = malloc(strlen(filename) + strlen(COPY_SUFF) + 1)) {
			strcpy(p, filename);
			strcat(p, COPY_SUFF);
			copy_file(filename, p);
			free(p);
		} else {
			warn("malloc");
		}
		file_copied = 1;
	}
	
	if ((fp = fopen(filename, "w")) == NULL) {
		warn("%s", filename);
		return;
	}
	fprintf(fp, "%s\n# Following entries added by tnamed\n", original_content);
	dbgprintf("in update_hosts_file()\n");
	print_hostname_map(m);
	for (i = 0; i < m->n; ++i) {
		in_addr.s_addr = m->map[i].ip_addr;
		fprintf(fp, "%s\t%s\n", inet_ntoa(in_addr), m->map[i].hostname);
	}
	fclose(fp);
}

void
delete_file(const char *filename) {
	if (unlink(filename) == -1)
		dbgprintf("%s: %s\n", filename, strerror(errno));
}

void
restore_hosts_file(void) {
	char *p;
	
	if (!opts.hosts_filename)
		return;
	
	if ((p = malloc(strlen(opts.hosts_filename) + strlen(COPY_SUFF) + 1)) == NULL) {
		dbgprintf("malloc: %s\n", strerror(errno));
		return;
	}
	strcpy(p, opts.hosts_filename);
	strcat(p, COPY_SUFF);
	
	copy_file(p, opts.hosts_filename);
	delete_file(p);
	
	free(p);
}

void
signal_handler(int sig) {
	switch (sig) {
	case SIGHUP:
		break;
	case SIGINT:
	case SIGTERM:
		restore_hosts_file();
		exit(0);
		break;
	}	
}

void
usage(void) {
	printf("usage: %s [-d] [hosts_file]\n", PROGNAME);
	exit(0);
}

void
proc_args(int argc, char **argv) {
	int i;
	
	opts.hosts_filename = DEFAULT_HOSTS_FILENAME;
	
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-d"))
			opts.dflag = 1;
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") || *(argv[i]) == '-')
			usage();
		else
			opts.hosts_filename = argv[i];
	}
}

int
main(int argc, char **argv) {
	struct my_info my_info;
	int i, s, n;
	char buf[BUFSIZE];
	fd_set fds;
	ssize_t ssize;
    char *p;
	struct hostname_mapping mapping;
	struct hostname_map map;
	struct in_addr in_addr;
	int handle_sigs[] = {
		SIGHUP,
		SIGINT,
		SIGTERM
	};
	struct {
		u_int8_t need_announce_self:1;
		u_int8_t need_make_request:1;
	} state;
	struct timeval interval;
	
	proc_args(argc, argv);
    
    if (!opts.dflag) {
        i = fork();
        if (i < 0)
            err(1, "fork");
        else if (i > 0)
            exit(0);
    }
	
	bzero(&my_info, sizeof(my_info));
	bzero(&map, sizeof(map));
	map.maxn = 1000;
	
	for (i = 0; i < sizeof(handle_sigs)/sizeof(*handle_sigs); ++i)
		signal(handle_sigs[i], signal_handler);
	
	s = listen_udp_port(DEFAULT_PORT);
	
	/* FD_ZERO(&fds); */
	
	state.need_announce_self = 1;
	state.need_make_request = 1;
	for ( ; ; ) {
		if (state.need_announce_self || state.need_make_request) {
			/* Sys configuration might've changed, retrieve current */
			free(my_info.ip_addrs);
			bzero(&my_info, sizeof(my_info));
			get_my_info(&my_info);
		}
		if (state.need_announce_self) {
			dbgprintf("announcing self\n");
			for (i = 0;	i < my_info.n_ip_addrs; ++i) {
				in_addr.s_addr = my_info.ip_addrs[i];
				dbgprintf("sending %s@%s\n", my_info.hostname, inet_ntoa(in_addr));
				encode_announce_msg(buf, my_info.hostname, my_info.ip_addrs[i]);
				broadcast_msg(buf, strlen(buf) + 1, my_info.ip_addrs[i]);
			}
			state.need_announce_self = 0;
		}
		if (state.need_make_request) {
			dbgprintf("making request\n");
			
			for (i = 0;	i < my_info.n_ip_addrs; ++i) {
				in_addr.s_addr = my_info.ip_addrs[i];
				encode_request_msg(buf);
				broadcast_msg(buf, strlen(buf) + 1, my_info.ip_addrs[i]);
			}
			state.need_make_request = 0;
		}
		
		dbgprintf("waiting for incoming data...\n");
		
		/*
		FD_SET(s, &fds);
		interval.tv_sec = 15;
		interval.tv_usec = 0;
		if ((n = select(1, &fds, &fds, &fds, &interval)) == -1)
			warn("select");
		
		if (!FD_ISSET(s, &fds)) {
			state.need_make_request = 1;
			continue;
		}
		*/
		
		if ((ssize = recv(s, buf, BUFSIZE, 0)) == -1) {
			warn("recv");
			continue;
		}
		dbgprintf("received something, size = %d\n", ssize);
		if (ssize < 2)
			continue;
		buf[ssize-1] = '\0';
		
		switch (buf[0]) {
		case 'R':
			dbgprintf("got a request\n");
			state.need_announce_self = 1;
			break;
		case 'A':
			if ((p = strchr(buf, '@')) == NULL) {
				dbgprintf("invalid announce msg: '@' not found");
				break;
			}
			*p = '\0';
			if (strchr(buf, '.') != NULL) {
				dbgprintf("invalid announce msg: hostname with dots");
				break;
			}
			if (strlen(buf + 1) >= HOSTNAMESIZE)
				buf[HOSTNAMESIZE-1] = '\0';
			if (!strcmp(my_info.hostname, buf + 1)) {
				dbgprintf("invalid announce msg: attempt to redefine our own hostname");
				break;
			}
			strcpy(mapping.hostname, buf + 1);
			if (inet_aton(p + 1, &in_addr) == -1 || !is_ordinary_ip(in_addr.s_addr)) {
				dbgprintf("invalid announce msg: invalid ip address");
				break;
			}
			mapping.ip_addr = in_addr.s_addr;
			dbgprintf("adding \"%s\"@%s\n", buf + 1, p + 1);
			add_mapping(&map, &mapping);
			print_hostname_map(&map);
			dbgprintf("updating hosts file\n");
			update_hosts_file(&map, opts.hosts_filename);
			break;
		}
	}
	
	exit(0);
}
