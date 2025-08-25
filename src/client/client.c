/*
** client.c -- a stream socket client demo
*/

#define _GNU_SOURCE

#include "http_builder.h"
#include "http_parser.h"
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

    // Create HTTP Request
    HTTP_MESSAGE request = init_http_message();
    strcpy(request.start_line.request.method, "GET");
    strcpy(request.start_line.request.request_target, "/");
    strcpy(request.start_line.request.protocol, "HTTP/1.1");

    // Set headers
    request.header_count = 4;
    strcpy(request.headers[0].key, "Host");
    strcpy(request.headers[0].value, argv[1]);
    strcpy(request.headers[1].key, "User-Agent");
    strcpy(request.headers[1].value, "TestClient/1.0");
    strcpy(request.headers[2].key, "Accept");
    strcpy(request.headers[2].value, "*/*");
    strcpy(request.headers[3].key, "Connection");
    strcpy(request.headers[3].value, "close");

    // Send the request
    build_and_send_message(&request, sockfd, REQUEST);

    // Recieve the HTTP Response
    HTTP_MESSAGE response = parse_http_message(sockfd, RESPONSE);
    print_http_message(&response);

    free_http_message(&response);
    close(sockfd);

    return 0;
}
