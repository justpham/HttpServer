/*
** server.c -- a stream socket server demo
*/

#define _GNU_SOURCE

#include "include/connect.h"
#include "ip_helper.h"
#include "macros.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(void)
{
    // listen on sock_fd, new connection on new_fd
    int sockfd, new_fd;

    sockfd = server_setup();

    while (1) { // main accept() loop
        if ((new_fd = accept_connection(sockfd)) == -1) {
            continue;
        }

        handle_client_connection(sockfd, new_fd, "Hello, world!");

        close(new_fd); // parent doesn't need this
    }

    return 0;
}
