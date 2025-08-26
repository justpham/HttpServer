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
server_router (HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{

    char *route = request->start_line.request.request_target;
    char method[MAX_METHOD_LENGTH] = { 0 };
    get_value_from_http_method(request->start_line.request.method, method, sizeof(method));

    response->start_line.response.protocol = request->start_line.request.protocol;

    if (strcmp(route, "/") == 0) {

        if (strcmp(method, "GET") == 0) {

            response->start_line.response.status_code = STATUS_OK;
            strcpy(response->start_line.response.status_message, "OK");

            response->header_count = 4;

            response->headers[0] = (HTTP_HEADER){ "Server", "HttpServer" };
            response->headers[1] = (HTTP_HEADER){ "Content-Type", "text/html; charset=UTF-8" };
            response->headers[2] = (HTTP_HEADER){ "Content-Length", "228" };
            response->headers[3] = (HTTP_HEADER){ "Connection", "close" };

            int html_file = open("html/index.html", O_RDONLY);

            http_message_set_body_fd(response, html_file, NULL, 228);

        } else {
            response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
            strcpy(response->start_line.response.status_message, "Method Not Allowed");

            response->header_count = 4;
            response->headers[0] = (HTTP_HEADER){ "Server", "HttpServer" };
            response->headers[1] = (HTTP_HEADER){ "Content-Length", "228" };
            response->headers[2] = (HTTP_HEADER){ "Allow", "GET" };
            response->headers[3] = (HTTP_HEADER){ "Connection", "close" };
        }

    } else {
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");

        response->header_count = 4;
        response->headers[0] = (HTTP_HEADER){ "Server", "HttpServer" };
        response->headers[1] = (HTTP_HEADER){ "Content-Type", "text/html; charset=UTF-8" };
        response->headers[2] = (HTTP_HEADER){ "Content-Length", "240" };
        response->headers[3] = (HTTP_HEADER){ "Connection", "close" };

        int html_file = open("html/NotFound.html", O_RDONLY);

        http_message_set_body_fd(response, html_file, NULL, 240);

    }

    return 0;
}



int
main(void)
{
    // listen on sock_fd, new connection on new_fd
    int sockfd, client_fd;

    sockfd = server_setup();

    while (1) { // main accept() loop
        if ((client_fd = accept_connection(sockfd)) == -1) {
            continue;
        }

        if (!fork()) {
            close(sockfd); // child doesn't need this

            HTTP_MESSAGE response = init_http_message();

            // Recieve HTTP Request
            HTTP_MESSAGE request = parse_http_message(client_fd, REQUEST);

            // Print out the request info for debug purposes
            print_http_message(&request, REQUEST);

            // Put Request in Router
            server_router(&request, &response);

            build_and_send_message(&response, client_fd, RESPONSE);

            // Clean everything up
            free_http_message(&request);
            close(client_fd);

            exit(0);
        }

        close(client_fd); // parent doesn't need this
    }

    return 0;
}
