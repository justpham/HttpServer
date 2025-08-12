/*
    Implementation for socket and connect abstractions
*/


#define _GNU_SOURCE

#include "connect.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
    Basic abstraction to connect to a valid host

    Parameters:
        const char *hostname - String with the address to connect to

    Returns a socket file descriptor on success, returns a negative error code on failure
*/
int connect_to_host(const char *hostname) {

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
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        print_ip("client: connecting to", p, true);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        print_ip("client: failed to connect to", p, false);
        return -2;
    }

    print_ip("client: successfully connected to", p, true);

    freeaddrinfo(servinfo);

    return sockfd;
}

/*
    Prints the IP
*/
void print_ip(char *message, struct addrinfo *p, bool stdout) {
    
    char s[INET6_ADDRSTRLEN];

    if (message == NULL) {
        message = "IP:";
    }

    // Get the address (doesn't matter if its IPv4 or IPv6)
    inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);

    if (stdout) printf("%s %s\n", message, s);
    else fprintf(stderr, "%s %s\n", message, s);
}

/*
    Recieves data from a given socket
*/
int recieve_data(int sockfd, char* buf, int maxLength) {

    int recieve_rate = RECV_RATE > (maxLength - 1) ? RECV_RATE : maxLength - 1;
    int numbytes = 0;

    if ((numbytes = recv(sockfd, buf, recieve_rate, 0)) == -1) {
        perror("recv() in recieve_data() failed\n");
        return -1;
    }

    buf[numbytes] = '\0';

    return 0;
}
