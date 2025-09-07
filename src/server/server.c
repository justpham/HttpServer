/*
** server.c -- a stream socket server demo
*/

#define _GNU_SOURCE

#include "http_builder.h"
#include "http_parser.h"
#include "include/connect.h"
#include "include/routes.h"
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
server_router(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{

    if (!request || !response) {
        fprintf(stderr, "Invalid parameters\n");
        return -1;
    }

    const char *route = request->start_line.request.request_target;
    int method = request->start_line.request.method;
    
    // Send back response always in HTTP 1.1
    response->start_line.response.protocol = HTTP_1_1;

    // Setup Routes
    if (strcmp(route, "/") == 0) {
        if (method == HTTP_GET) {
            default_handler(response, response);
        } else {
            response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
            strcpy(response->start_line.response.status_message, "Method Not Allowed");
            add_header(response, "Allow", "GET");
        }
    } else if (strcmp(route, "/echo") == 0) {
        if (method == HTTP_POST) {
            echo_handler(request, response);
        } else {
            response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
            strcpy(response->start_line.response.status_message, "Method Not Allowed");
            add_header(response, "Allow", "POST");
        }
    } else if (strncmp(route, "/static", 7) == 0) {
        if (method == HTTP_GET) {
            static_handler(request, response);
        }
    } else {
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
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

            srand((unsigned int) time(NULL));

            HTTP_MESSAGE response = init_http_message();

            // Default Headers
            add_header(&response, "Server", "HttpServer");
            add_header(&response, "Connection", "close");

            // Recieve HTTP Request
            HTTP_MESSAGE request = init_http_message();
            if (parse_http_message(&request, client_fd, REQUEST) != 0) {
                fprintf(stderr, "Failed to parse HTTP request\n");
                // TODO: Move this error message inside parse_http_message to get more details about
                // errors
                build_error_response(&response, STATUS_BAD_REQUEST, "Bad Request", NULL);
                close(client_fd);
                exit(1);
            } else {
                print_http_message(&request, REQUEST);
                server_router(&request, &response);
            }

            print_http_message(&response, RESPONSE);

            if (build_and_send_message(&response, client_fd, RESPONSE) != 0) {
                fprintf(stderr, "Failed to send HTTP response to client successfully\n");
            }

            // Clean everything up
            free_http_message(&request);
            free_http_message(&response);
            close(client_fd);

            exit(0);
        }

        close(client_fd); // parent doesn't need this
    }

    return 0;
}
