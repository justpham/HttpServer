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
    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    int sockfd = connect_to_host(argv[1]);

    // Send a valid HTTP Request with additional headers
    const char *dummy_http_request = "GET / HTTP/1.1\r\n"
                                     "Host: example.com\r\n"
                                     "User-Agent: TestClient/1.0\r\nAccept:";

    const char *dummy_http_request_2 = " */*\r\n"
                                       "X-Dummy-Header: hello-world\r\n"
                                       "Content-Length: 20\r\n"
                                       "\r\n"
                                       "Sample Body!!!\n";

    // TODO: Setup streaming test....

    printf("Sending HTTP request:\n%s", dummy_http_request);

    send(sockfd, dummy_http_request, strlen(dummy_http_request), 0);

    printf("Sleeping...\n");

    printf("Sending additional data...\n");

    sleep(5);

    printf("Sending HTTP request:\n%s", dummy_http_request_2);

    send(sockfd, dummy_http_request_2, strlen(dummy_http_request_2), 0);

    sleep(5);

    printf("Sending hello\n");

    send(sockfd, "hello", 5, 0);

    close(sockfd);

    return 0;
}
