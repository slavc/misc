#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "findmachine.h"

int
main(int argc, char **argv)
{
    int s;
    struct sockaddr_in sin, peer;
    socklen_t peer_len;
    unsigned char buf[1];
    char hostname[HOST_NAME_MAX + 1];
    int hostname_len;


    /* Get the hostname */

    if (gethostname(hostname, sizeof(hostname)))
        err(1, "gethostname");
    hostname[sizeof(hostname) - 1] = '\0';
    hostname_len = strlen(hostname);


    /* Listen on an UDP socket on all interfaces */

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
        err(1, "socket");

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (const struct sockaddr *) &sin, sizeof(sin)))
        err(1, "bind");

    for ( ;; ) {
        /* When we receive anything at all, respond with our hostname */

        peer_len = sizeof(peer);
        if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &peer, &peer_len) < 0)
            err(1, "recvfrom");

        if (sendto(s, hostname, hostname_len, 0, (const struct sockaddr *) &peer, peer_len) < 0)
            err(1, "recvfrom");
    }

    close(s);

    return 0;
}

