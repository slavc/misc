#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "findmachine.h"

#define IP_ADDR(a, b, c, d) htonl(((((a) << 8) | (b)) << 8 | (c)) << 8 | d)

int
main(int argc, char **argv)
{
    int s;
    struct sockaddr_in sin, peer;
    socklen_t peer_len;
    const char *hostname;
    struct {
        unsigned int a;
        unsigned int b;
        unsigned int c;
        unsigned int d;
    } ip;
    char buf[HOST_NAME_MAX + 1];

    if (argc < 3) {
        usage();
        return 1;
    }

    if (sscanf(argv[1], "%u.%u.%u", &ip.a, &ip.b, &ip.c) != 3) {
        usage();
        return 1;
    }
    hostname = argv[2];

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
        err(1, "socket");

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (const struct sockaddr *) &sin, sizeof(sin)))
        err(1, "bind");

#define MIN_ADDR 1
#define MAX_ADDR 255

    for (ip.d = MIN_ADDR; ip.d < MAX_ADDR; ++ip.d) {
        bzero(&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(PORT);
        sin.sin_addr.s_addr = htonl(ip.a, ip.b, ip.c, ip.d);

        /* It doesn't matter what exactly we send, just throw a byte */
        if (sendto(s, buf, 1, (const struct sockaddr *) &sin, sizeof(sin)))
            err(1, "sendto");
    }

    /* ???????? */

    close(s);

    return 0;
}

void
usage(void)
{
        printf("usage: findmachine <subnet> <hostname>\n");
}
