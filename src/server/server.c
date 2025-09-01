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
echo_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{

    // Read the request body
    const char *content_type
        = get_header_value(request->headers, MAX_HEADER_LENGTH, "Content-Type");

    if (strcmp("text/plain", content_type) == 0) {

        ssize_t bytes_read = 0;

        char *file_contents = malloc(request->body_length + 1);

        if (!file_contents) {
            perror("Failed to allocate memory");
            return -1;
        }

        print_http_message(request, REQUEST);

        if ((bytes_read = read(request->body_fd, file_contents, request->body_length)) == -1) {
            perror("Failed to read from temp file");
            close(request->body_fd);
            free(file_contents);
            build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error",
                                 NULL);
            return -1;
        }

        // Pass it to the response
        add_header(response, "Content-Type", content_type);

        http_message_open_temp_file(response, bytes_read);

        if (write(response->body_fd, file_contents, bytes_read) == -1) {
            perror("Failed to write to temp file");
            close(response->body_fd);
            free(file_contents);
            build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error",
                                 NULL);
            return -1;
        }

        response->start_line.response.status_code = STATUS_OK;
        strcpy(response->start_line.response.status_message, "OK");
        free(file_contents);

    } else {
        // Unsupported media type
        response->start_line.response.status_code = STATUS_UNSUPPORTED_MEDIA_TYPE;
        strcpy(response->start_line.response.status_message, "Unsupported Media Type");
        http_message_open_existing_file(response, "html/UnsupportedMediaType.html", O_RDONLY,
                                        false);
        return -1;
    }

    return 0;
}

int
static_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{
    if (!request || !response) {
        build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", NULL);
        return -1;
    }

    printf("Static file request: %s\n", request->start_line.request.request_target);

    // Get route
    char route[MAX_TARGET_LENGTH + 1] = { 0 }; // +1 for the leading '.'
    size_t target_len = strlen(request->start_line.request.request_target);
    if (target_len >= MAX_TARGET_LENGTH) {
        printf("Request target too long: %s\n", request->start_line.request.request_target);
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return -1;
    }
    snprintf(route, sizeof route, ".%s", request->start_line.request.request_target);

    int method = request->start_line.request.method;

    response->start_line.response.protocol = request->start_line.request.protocol;

    // Get the absolute path of the requested file
    char resolved_path[MAX_TARGET_LENGTH];
    if (realpath(route, resolved_path) == NULL) {
        printf("Failed to resolve path: %s\n", route);
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return -1;
    }

    // Get the absolute path of the static directory
    char static_dir_resolved[MAX_TARGET_LENGTH];
    if (realpath(STATIC_PATH_STR, static_dir_resolved) == NULL) {
        printf("Failed to resolve static directory path: %s\n", STATIC_PATH_STR);
        response->start_line.response.status_code = STATUS_FORBIDDEN;
        strcpy(response->start_line.response.status_message, "Forbidden");
        http_message_open_existing_file(response, "html/Forbidden.html", O_RDONLY, false);
        return -1;
    }

    // Check that its a file rather than a directory
    struct stat path_stat;
    if (stat(resolved_path, &path_stat) != 0) {
        printf("Failed to stat path: %s\n", resolved_path);
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return -1;
    }
    if (S_ISDIR(path_stat.st_mode)) {
        printf("Requested path is a directory: %s\n", resolved_path);
        response->start_line.response.status_code = STATUS_FORBIDDEN;
    }

    // Check if the resolved path is still under the static directory
    if (strncmp(resolved_path, static_dir_resolved, strlen(static_dir_resolved)) != 0) {
        printf("Access denied: %s\n", resolved_path);
        response->start_line.response.status_code = STATUS_FORBIDDEN;
        strcpy(response->start_line.response.status_message, "Forbidden");
        http_message_open_existing_file(response, "html/Forbidden.html", O_RDONLY, false);
        return -1;
    }

    // Only accept GET requests
    if (method != HTTP_GET) {
        printf("Method not allowed: %d\n", method);
        response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
        strcpy(response->start_line.response.status_message, "Method Not Allowed");
        add_header(response, "Allow", "GET");
        return -1;
    }

    // File is accessible
    response->start_line.response.status_code = STATUS_OK;
    strcpy(response->start_line.response.status_message, "OK");
    http_message_open_existing_file(response, resolved_path, O_RDONLY, true);
    return 0;
}

int
server_router(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{

    if (!request || !response) {
        fprintf(stderr, "Invalid parameters\n");
        return -1;
    }

    const char *route = request->start_line.request.request_target;
    int method = request->start_line.request.method;

    response->start_line.response.protocol = request->start_line.request.protocol;

    printf("Routing request: %s %s\n", method == HTTP_GET ? "GET" : "POST", route);

    if (strcmp(route, "/") == 0) {

        if (method == HTTP_GET) {

            response->start_line.response.status_code = STATUS_OK;
            strcpy(response->start_line.response.status_message, "OK");
            http_message_open_existing_file(response, "html/index.html", O_RDONLY, false);

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
            }

            // Print out the request info for debug purposes
            print_http_message(&request, REQUEST);

            // Put Request in Router
            server_router(&request, &response);

            print_http_message(&response, RESPONSE);

            if (build_and_send_message(&response, client_fd, RESPONSE) != 0) {
                fprintf(stderr, "Failed to send HTTP response to client successfully\n");
            }

            // Clean everything up
            free_http_message(&request);
            close(client_fd);

            exit(0);
        }

        close(client_fd); // parent doesn't need this
    }

    return 0;
}
