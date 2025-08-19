/*
** client.c -- a stream socket client demo
*/

#define _GNU_SOURCE

#include "include/connect.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXDATASIZE 100

int
main(int argc, char *argv[])
{
    char buf[MAXDATASIZE];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    int sockfd = connect_to_host(argv[1]);

    receive_data(sockfd, buf, MAXDATASIZE);

    printf("client: received '%s'\n", buf);

    close(sockfd);

    return 0;
}
