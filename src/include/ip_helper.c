
#define _GNU_SOURCE

#include "ip_helper.h"

/*
    get sockaddr, IPv4 or IPv6
*/
void *
get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

/*
    Prints the IP for the sockaddr struct
*/
void
print_ip(char *message, int ai_family, struct sockaddr *address, bool stdout)
{

    char s[INET6_ADDRSTRLEN];

    if (message == NULL) {
        message = "IP:";
    }

    // Get the address (doesn't matter if its IPv4 or IPv6)
    inet_ntop(ai_family, get_in_addr(address), s, sizeof s);

    if (stdout)
        printf("%s %s\n", message, s);
    else
        fprintf(stderr, "%s %s\n", message, s);
}
