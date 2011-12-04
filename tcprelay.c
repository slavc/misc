/* Kind of like a very simple IP level proxy.
 *
 * Given a listen address and destination address, whenever someone connects
 * to listen address -- tcprelay connects to destination address and just
 * sends everything it receives on the listen address to destination address
 * and vice-versa.
 *
 * Optionally, it can print the communication between hosts to stdout, which
 * could be useful for debugging or exploration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define NELEMS(array) (sizeof(array)/sizeof(array[0]))

#define DBG_STREAM stdout

int Dflag; /* run in foreground and produce debug output */

void usage(void);
int dbgprintf(int level, const char *fmt, ...);
void tcprelay(const char *from, const char *to);
int poll_loop_body(int fd_index, int n_socks, struct pollfd *socks, const char **str_addrs);
int listen_at(const char *addr_and_port);
int connect_to_dest(const char *addr_and_port);
int pump(int from, int to);

int
main(int argc, char **argv) {
    int ch;
    const char *listen_addr_and_port, *dest_addr_and_port;

    while ((ch = getopt(argc, argv, "l:t:D")) != -1) {
        switch (ch) {
        case 'l':
            listen_addr_and_port = optarg;
            break;
        case 't':
            dest_addr_and_port = optarg;
            break;
        case 'D':
            ++Dflag;
            break;
        default:
            usage();
            break;
        }
    }

    if (!(listen_addr_and_port && dest_addr_and_port))
        usage();

    if (!Dflag) {
        if (daemon(0, 0) == -1)
            err(1, "daemon");
    }

    tcprelay(listen_addr_and_port, dest_addr_and_port);

    exit(EXIT_SUCCESS);
}

void
usage(void) {
    printf("usage: tcprelay -l xx.xx.xx.xx:<port> -t xx.xx.xx.xx:<port> [-D]\n"
           "Repeat everything received in -l addr to -t addr and vice versa.\n"
           "  -l -- address and port at which to listen to\n"
           "  -t -- address and port where to connect to, when accepted\n"
           "        connection at -l address\n"
           "  -D -- run in foreground, outputting debug messages;\n"
           "        specify twice to also output raw communication between hosts\n");
    exit(EXIT_SUCCESS);
}

int
dbgprintf(int level, const char *fmt, ...) {
    va_list ap;
    int n;

    if (level > Dflag)
        return 0;

    va_start(ap, fmt);
    n = vfprintf(DBG_STREAM, fmt, ap);
    va_end(ap);

    return n;
}
    
void
tcprelay(const char *listen_addr, const char *dest_addr) {
    int listen_s, listen_con;
    struct sockaddr_storage listen_sa;
    socklen_t listen_sa_len = sizeof listen_sa;
    struct sockaddr_in *sin;
    int dest_con;
    struct pollfd socks[2];
    int rc;
    char accept_addr_and_port[sizeof "255.255.255.255:65535"] = "";
    const char *str_addrs[] = { accept_addr_and_port, dest_addr };

    if ((listen_s = listen_at(listen_addr)) == -1)
        errx(1, "listen_at %s", listen_addr);

    for ( ;; ) {
        if ((listen_con = accept(listen_s, (struct sockaddr *) &listen_sa, &listen_sa_len)) == -1) {
            warn("accept");
            continue;
        }
        sin = (struct sockaddr_in *) &listen_sa;
        snprintf(accept_addr_and_port, sizeof accept_addr_and_port, "%s:%i",
            inet_ntoa(sin->sin_addr),
            (int) ntohs(sin->sin_port));
        dbgprintf(1, "# accepted connection from %s\n", str_addrs[0]);

        if ((dest_con = connect_to_dest(dest_addr)) == -1)
            errx(1, "connect_to_dest %s", dest_addr);
        dbgprintf(1, "# connected to %s\n", dest_addr);

        socks[0].fd = listen_con;
        socks[0].events = POLLIN;
        socks[1].fd = dest_con;
        socks[1].events = POLLIN;

        while ((rc = poll(socks, NELEMS(socks), 0)) != -1) {
            if (poll_loop_body(0, 2, socks, str_addrs))
                break;
            if (poll_loop_body(1, 2, socks, str_addrs))
                break;
        }
        if (rc == -1)
            warn("poll");

        close(listen_con);
        close(dest_con);
    }
}

int
poll_loop_body(int fd_index, int n_socks, struct pollfd *socks, const char **str_addrs) {
    int i;
    ssize_t nbytes;

    if (socks[fd_index].revents & (POLLHUP | POLLNVAL)) {
        for (i = 0; i < n_socks; ++i)
            shutdown(socks[i].fd, SHUT_RDWR);
        return 1;
    }

    if (socks[fd_index].revents & POLLERR) {
        for (i = 0; i < n_socks; ++i)
            shutdown(socks[i].fd, SHUT_RDWR);
        return 1;
    }

    if (socks[fd_index].revents & POLLIN) {
        for (i = 0; i < n_socks; ++i) {
            if (i == fd_index)
                continue;
            dbgprintf(1, "# %s -> %s\n", str_addrs[fd_index], str_addrs[i]);
            nbytes = pump(socks[fd_index].fd, socks[i].fd);
            dbgprintf(2, "\n\n");
            if (nbytes == -1) {
                warnx("pump");
            } else if (nbytes == 0) {
                dbgprintf(1, "# one of ends closed connection, closing sockets, awaiting new connection...\n");
                for (i = 0; i < n_socks; ++i)
                    shutdown(socks[i].fd, SHUT_RDWR);
                return 1;
            }
        }
    }

    return 0;
}

int
listen_at(const char *addr_and_port) {
    int s;
    char *buf, *p;
    struct sockaddr_in sin;

    assert(addr_and_port && "Missing mandatory argument");

    bzero(&sin, sizeof(sin));

    if ((buf = strdup(addr_and_port)) == NULL)
        err(1, "strdup");

    if ((p = strchr(buf, ':')) == NULL)
        return -1;
    *p++ = '\0';

    sin.sin_port = htons((in_port_t) atoi(p));
    if (inet_aton(buf, &sin.sin_addr) == -1) {
        free(buf);
        return -1;
    }
    free(buf);
    sin.sin_family = AF_INET;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        warn("socket");
        return -1;
    }
    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
        warn("bind");
        return -1;
    }
    if (listen(s, 0) == -1) {
        warn("listen");
        return -1;
    }

    return s;
}

int
connect_to_dest(const char *addr_and_port) {
    int s;
    char *buf, *p;
    struct sockaddr_in sin;

    bzero(&sin, sizeof(sin));

    if ((buf = strdup(addr_and_port)) == NULL)
        err(1, "strdup");
    if ((p = strchr(buf, ':')) == NULL)
        return -1;
    *p++ = '\0';

    sin.sin_port = htons((in_port_t) atoi(p));
    if (inet_aton(buf, &sin.sin_addr) == -1) {
        free(buf);
        return -1;
    }
    free(buf);
    sin.sin_family = AF_INET;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        warn("socket");
        return -1;
    }

    if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
        warn("connect");
        return -1;
    }

    return s;
}

int
pump(int from, int to) {
    static char buf[16384];
    const size_t buf_size = sizeof(buf);
    ssize_t nbytes;

    while ((nbytes = read(from, buf, buf_size)) > 0) {
        if (Dflag >= 2)
            write(fileno(DBG_STREAM), buf, nbytes);
        if (write(to, buf, nbytes) == -1) {
            warn("write");
            return -1;
        }
        if (nbytes != buf_size)
            break;
    }
    if (nbytes == -1)
        warn("read");

    return nbytes;
}
