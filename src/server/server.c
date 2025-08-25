/*
** server.c -- a stream socket server demo
*/

#define _GNU_SOURCE

#include "http_builder.h"
#include "http_parser.h"
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

        if (!fork()) {
            close(sockfd); // child doesn't need this

            // Recieve HTTP Request
            HTTP_MESSAGE request = parse_http_message(new_fd, REQUEST);

            // Print out the request info for debug purposes
            print_http_message(&request);

            // Handle HTTP Headers

            // Handle HTTP Body

            // Create HTTP Response
            HTTP_MESSAGE response = init_http_message();

            printf("Response:\n");

            strcpy(response.start_line.response.protocol, request.start_line.request.protocol);
            strcpy(response.start_line.response.status_code, "200");
            strcpy(response.start_line.response.status_message, "OK");

            printf("Response:\n");
            print_http_message(&response);

            response.header_count = 4;
            response.headers[0] = (HTTP_HEADER){ "Server", "HttpServer" };
            response.headers[1] = (HTTP_HEADER){ "Content-Type", "text/html; charset=UTF-8" };
            response.headers[2] = (HTTP_HEADER){ "Content-Length", "228" };
            response.headers[3] = (HTTP_HEADER){ "Connection", "close" };

            int html_file = open("html/index.html", O_RDONLY);

            http_message_set_body_fd(&response, html_file, NULL, 228);

            print_http_message(&response);

            // Send HTTP Response
            build_and_send_message(&response, new_fd, RESPONSE);

            // Clean everything up
            free_http_message(&request);
            close(new_fd);

            exit(0);
        }

        close(new_fd); // parent doesn't need this
    }

    return 0;
}
