#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>

#define DEFAULT_PORT 1234

extern char *optarg;
extern int optind, opterr, optopt;

int
main(int argc, char **argv)
{
    int              ch;
    extern int       optind;
    int              listen_mode;
    const char      *addr = NULL;
    unsigned short   port;

    listen_mode = 0;
    port = DEFAULT_PORT;

    while ((ch = getopt(argc, argv, "hl")) != -1) {
        switch (ch) {
        case 'h':
            usage();
            exit(0);
        case 'l':
            listen_mode = 1;
            break;
        default:
            usage();
            exit(1);
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc > 0) {
        addr = *argv;
        --argc;
        ++argv;
    }

    if (argc > 0) {
        if (sscanf(*argv, "%hu", &port) < 1) {
            usage();
            exit(1);
        }
    }

    if (listen_mode)
        findmachined(port);
    else {
        if (addr == NULL) {
            usage();
            exit(1);
        }
        findmachine(addr, port);
    }

    return 0;
}

void
findmachined(unsigned short port)
{
    int                  s;
    struct sockaddr_in   my_addr, peer_addr;
    size_t               peer_addr_size;
    char                 rcvbuf[1];
    const size_t         rcvbuf_size = sizeof(rcvbuf);
    char                *sndbuf = NULL;
    size_t               sndbuf_size = 0;
    ssize_t              n_bytes;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
        err(1, "socket");

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_port = hton(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, &my_addr, sizeof(my_addr)))
        err(1, "bind");

    for ( ;; ) {
        peer_addr_size = sizeof(peer_addr);
        n_bytes = recvfrom(s, rcvbuf, rcvbuf_size, &peer_addr, &peer_addr_size);
        if (n_bytes == -1)
            err(1, "recvfrom");
        else if (n_bytes == 0)
            continue;

        /* TODO get hostname and the list of our ip addresses and reply */
    }
        
    return 0;
}
