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

#include <sys/epoll.h>

#define MAX_EPOLL_EVENTS    16
#define MAX_CONNECTIONS     MAX_EPOLL_EVENTS

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
accept_loop(int server_fd, int epoll_fd)
{
    int client_fd;

    while (1) {
        client_fd = accept_connection (server_fd);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* no more pending connections */
                return 0;
            } else {
                perror("accept");
                return -1;
            }
        }
        
        // Set client_fd to nonblocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl F_GETFL");
            return -1;
        }
        if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl F_SETFL");
            close(client_fd);
            return -1;
        }

        // Add to epoll and listen for read and write events
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl ADD client");
            close(client_fd);
            return -1;
        }
    }

    return 0;

}


int
epoll_implementation(void)
{

    // TODO: Add a connection map (an array with special struct)
    struct epoll_event events[MAX_EPOLL_EVENTS];            // Buffer for epoll_wait()
    struct epoll_event ev;
    int num_events;

    int epoll_fd;
    int server_fd;

    // Setup server
    server_fd = server_setup();
    if (server_fd == - 1) {
        printf(stderr, "Server setup failed.");
        return -1;
    }

    // Set listening socket to non blocking so epoll can continue
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        close(server_fd);
        return -1;
    }

    // Setup EPOLL
    epoll_fd = epoll_create1(0);

    // Add listening socket to EPOLL
    ev.data.fd = server_fd;
    ev.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror ("epoll_ctl for listening socket:");
        close(server_fd);
        return -1;
    }

    while (1) {
        if ((num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1)) == -1) {
            perror("epoll_wait: ");
            // TODO: Close active locations
            return -1;
        };

        // Handle events
        for (int event_iter = 0; event_iter < num_events; event_iter++) {

            struct epoll_event curr_event = events[event_iter];
            int curr_fd = curr_event.data.fd;

            // Recieve a new request 
            if (curr_fd == server_fd && curr_event.events & EPOLLIN) {
                if (accept_loop(server_fd, epoll_fd) == -1) {
                    perror("accept_loop");
                }
                continue;
            }

            // TODO: Write Operation (Finish a write operation)
            if (curr_event.events & EPOLLOUT) {
                continue;
            }

            // Read Operation
            if (curr_event.events & EPOLLIN) {

                HTTP_MESSAGE response = init_http_message();
                HTTP_MESSAGE request = init_http_message();

                add_header(&response, "Server", "HttpServer");

                if (parse_http_message(&request, curr_fd, REQUEST) != 0) {
                    fprintf(stderr, "Failed to parse HTTP request\n");
                    build_error_response(&response, STATUS_BAD_REQUEST, "Bad Request", NULL);
                    close(curr_fd);
                } else {
                    print_http_message(&request, REQUEST);
                    server_router(&request, &response);
                }

                print_http_message(&response, RESPONSE);

                if (build_and_send_message(&response, curr_fd, RESPONSE) != 0) {
                    fprintf(stderr, "Failed to send HTTP response to client successfully\n");
                }

                if (strcmp(get_header_value(&request.headers, request.header_count, "Connection"), "close") == 0) {
                    close(curr_fd);
                }

                // Assume that we recieve and send the full messages.
                free_http_message(&request);
                free_http_message(&response);
                continue;

            }

            // Close connection
            if (curr_event.events & EPOLLRDHUP) {
                close(curr_fd);
                continue;

            }

            if (curr_event.events & (EPOLLERR | EPOLLHUP)) {
                int err = 0;
                socklen_t errlen = sizeof(err);
                if (getsockopt(curr_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
                    perror("getsockopt");
                } else if (err) {
                    fprintf(stderr, "socket error on fd %d: %s\n", curr_fd, strerror(err));
                } else {
                    fprintf(stderr, "hangup on fd %d\n", curr_fd);
                }
                close(curr_fd);
                continue;
            }

        }


    }

    // TODO Flush out fds when terminating 



    return 0;

}

int main(void)
{
    return epoll_implementation();
}