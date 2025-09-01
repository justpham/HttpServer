/*
    Implementation for socket and connect abstractions for a client
*/

#define _GNU_SOURCE

#include "connect.h"

/*
    Basic abstraction to connect to a valid host

    Parameters:
        const char *hostname - String with the address to connect to

    Returns a socket file descriptor on success, returns a negative error code on failure
*/
int
connect_to_host(const char *hostname)
{

    int rv;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Get host info
    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        print_ip("client: connecting to", p->ai_family, p->ai_addr, true);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -2;
    }

    print_ip("client: successfully connected to", p->ai_family, p->ai_addr, true);

    freeaddrinfo(servinfo);

    return sockfd;
}
