/*
** server.c -- a stream socket server demo
*/

#define _GNU_SOURCE

#include "conn_map.h"
#include "http_builder.h"
#include "http_lib.h"
#include "http_parser.h"
#include "include/connect.h"
#include "include/routes.h"
#include "ip_helper.h"
#include "macros.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <sys/epoll.h>

#define SERVER_NAME "HttpServer"

#define MAX_EPOLL_EVENTS 16
#define MAX_CONNECTIONS 64
#define TIMEOUT_LIMIT 999999999
#define ACTIONS_LIMIT 1000

#define RECV_EPOLL_FLAGS EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET
#define SEND_EPOLL_FLAGS EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET

static volatile sig_atomic_t shutdown_requested = 0;

void
cleanup_connection(struct conn *connection_map, int fd, int max_connections, int epoll_fd)
{
    // Remove from epoll first to prevent future events
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    // Then remove from connection map (this will close the fd)
    remove_conn_from_map(connection_map, fd, max_connections);
}

int
send_error_response(HTTP_MESSAGE *response, int fd, struct conn *map, int epoll_fd)
{
    if (response == NULL) {
        fprintf(stderr, "send_error_response(): response is NULL!\n");
        return -1;
    }

    add_header(response, "Connection", "close");

    int ret;
    struct conn *curr_conn = get_conn(map, fd, MAX_CONNECTIONS);

    // Set socket to blocking
    int flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    set_conn_state(curr_conn, SENDING_HEADERS);
    ret = build_and_send_headers(response, fd, curr_conn->offset, false, RESPONSE);
    if (ret < 0) {
        fprintf(stderr, "Failed to send HTTP headers to client\n");
        // Special case where we can't send an HTTP message to the client, so simply close the fd.
        cleanup_connection(map, fd, MAX_CONNECTIONS, epoll_fd);
        return -1;
    }

    int body_length = get_file_length(response->body_fd);

    if (body_length > 0) {
        set_conn_state(curr_conn, SENDING_BODY);
        ret = build_and_send_body(response, fd);
        if (ret < 0) {
            fprintf(stderr, "Failed to send HTTP headers to client\n");
            // Special case where we can't send an HTTP message to the client, so simply close the
            // fd.
            cleanup_connection(map, fd, MAX_CONNECTIONS, epoll_fd);
            return -1;
        }
    }

    cleanup_connection(map, fd, MAX_CONNECTIONS, epoll_fd);

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

    // Send back response always in HTTP 1.1
    response->start_line.response.protocol = HTTP_1_1;

    if (!route || strlen(route) == 0) {
        response->start_line.response.status_code = STATUS_BAD_REQUEST;
        strcpy(response->start_line.response.status_message, "Bad Request");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return 0;
    }

    // Setup Routes
    if (strcmp(route, "/") == 0) {
        if (method == HTTP_GET) {
            default_handler(request, response);
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
    } else if (strcmp(route, "/favicon.ico") == 0) {
        if (method == HTTP_GET) {
            favicon_handler(request, response);
        } else {
            response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
            strcpy(response->start_line.response.status_message, "Method Not Allowed");
            add_header(response, "Allow", "GET");
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
accept_loop(int server_fd, int epoll_fd, struct conn *map)
{
    int client_fd;

    while (get_conn_map_length(map, MAX_CONNECTIONS) < MAX_CONNECTIONS) {
        client_fd = accept_connection(server_fd);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* no more pending connections */
                fprintf(stderr, "Recieved an EAGAIN signal\n");
                break;
            } else {
                return -1;
            }
        }

        // Add this debug check
        if (client_fd == 0) {
            fprintf(stderr, "WARNING: accept_connection returned FD 0 (stdin)!\n");
            close(client_fd);
            continue;
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
        ev.events = RECV_EPOLL_FLAGS;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl ADD client");
            close(client_fd);
            return -1;
        } else {
            fprintf(stderr, "DEBUG: Added FD %d to epoll\n", client_fd);
        }

        add_conn_to_map(map, client_fd, MAX_CONNECTIONS);
        struct conn *client_conn = get_conn(map, client_fd, MAX_CONNECTIONS);
        set_conn_state(client_conn, IDLE);
        update_conn_time(client_conn);

        fprintf(stderr, "accept_loop(): Added FD %d to server\n", client_fd);
    }

    /*
        Current policy is to reject any new connections if the server is full.
    */
    while (
        client_fd != -1 && 
        get_conn_map_length(map, MAX_CONNECTIONS) >= MAX_CONNECTIONS
    ) {
        client_fd = accept_connection(server_fd);
        if (errno == EAGAIN || errno == EWOULDBLOCK || client_fd == -1) {
            /* no more pending connections */
            break;
        }
        // Server is full, immediately close the connection, don't care to receive request from client
        close(client_fd);
        fprintf(stderr, "Server full, rejected connection on FD %d\n", client_fd);
    }

    return 0;
}

void
unwind_server(int sig)
{
    (void) sig;
    shutdown_requested = 1;
}

int
epoll_implementation(void)
{

    struct conn connection_map[MAX_CONNECTIONS];
    struct epoll_event events[MAX_EPOLL_EVENTS]; // Buffer for epoll_wait()
    struct epoll_event ev;
    int num_events;

    int epoll_fd;
    int server_fd;

    signal(SIGINT, unwind_server);

    initialize_conn_map(connection_map, MAX_CONNECTIONS);

    // Setup server
    server_fd = server_setup();

    fprintf(stderr, "DEBUG: server_setup() returned FD %d\n", server_fd);

    if (server_fd == -1) {
        fprintf(stderr, "Server setup failed.\n");
        return -1;
    }

    add_conn_to_map(connection_map, server_fd, MAX_CONNECTIONS);

    fprintf(stderr, "DEBUG: Added server FD %d to connection map\n", server_fd);
    fprintf(stderr, "DEBUG: Connection map slot 0 has FD %d\n", connection_map[0].fd);

    // Set listening socket to non blocking so epoll can continue
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        remove_conn_from_map(connection_map, server_fd, MAX_CONNECTIONS);
        return -1;
    }

    // Setup EPOLL
    epoll_fd = epoll_create1(0);

    // Add listening socket to EPOLL
    ev.data.fd = server_fd;
    ev.events = EPOLLIN | EPOLLET;

    fprintf(stderr, "Adding Server FD %d to EPOLL\n", server_fd);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl for listening socket:");
        remove_conn_from_map(connection_map, server_fd, MAX_CONNECTIONS);
        return -1;
    }

    while (1) {

        if (shutdown_requested) {
            break;
        }

        if ((num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1)) == -1) {
            perror("epoll_wait: ");
            unwind_server(SIGINT);
            continue;
        };

        // Handle events
        for (int event_iter = 0; event_iter < num_events; event_iter++) {

            struct epoll_event curr_event = events[event_iter];
            int curr_fd = curr_event.data.fd;
            struct conn *curr_conn = get_conn(connection_map, curr_fd, MAX_CONNECTIONS);

            fprintf(stderr, "DEBUG: Processing epoll event for FD %d, events=0x%x\n", curr_fd,
                    curr_event.events);

            if (curr_conn == NULL) {
                fprintf(stderr, "Unknown FD (%d) for event iter %d\n", curr_fd, event_iter);
                // Add this debug to see if FD 0 is anywhere in the connection map
                fprintf(stderr, "DEBUG: Checking if FD %d exists in connection map:\n", curr_fd);
                for (int i = 0; i < MAX_CONNECTIONS; i++) {
                    if (connection_map[i].fd == curr_fd) {
                        fprintf(stderr, "DEBUG: Found FD %d at index %d\n", curr_fd, i);
                    }
                }
                // Try to remove from epoll before closing
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
                continue;
            }

            curr_conn->action_count++;

            update_conn_time(curr_conn);

            // Recieve a new request
            if (curr_fd == server_fd && curr_event.events & EPOLLIN) {
                if (accept_loop(server_fd, epoll_fd, connection_map) == -1) {
                    perror("accept_loop");
                }
                continue;
            }

            // Finish a write operation
            else if (curr_event.events & EPOLLOUT) {

                int ret = 0;
                int original_state = curr_conn->state;

                if (curr_conn->request == NULL) {
                    // The request should also be built
                    if (curr_conn->response == NULL) {
                        shallow_copy_http_message_to_conn(curr_conn, init_http_message(), RESPONSE);
                    }
                    build_error_response(curr_conn->response, STATUS_INTERNAL_SERVER_ERROR,
                                         "Internal Server Error", NULL);
                    send_error_response(curr_conn->response, curr_fd, connection_map, epoll_fd);
                    continue;
                }

                if (curr_conn->response == NULL) {
                    // The response should be built
                    shallow_copy_http_message_to_conn(curr_conn, init_http_message(), RESPONSE);
                    build_error_response(curr_conn->response, STATUS_INTERNAL_SERVER_ERROR,
                                         "Internal Server Error", NULL);
                    send_error_response(curr_conn->response, curr_fd, connection_map, epoll_fd);
                    continue;
                }

                HTTP_MESSAGE *response = curr_conn->response;

                switch (curr_conn->state) {

                // These events do not send any data so send an error
                case IDLE:
                    [[fallthrough]];
                case PARSING_HEADERS:
                    [[fallthrough]];
                case PARSING_BODY:
                    shallow_copy_http_message_to_conn(curr_conn, init_http_message(), RESPONSE);
                    build_error_response(curr_conn->response, STATUS_INTERNAL_SERVER_ERROR,
                                         "Internal Server Error", NULL);
                    send_error_response(curr_conn->response, curr_fd, connection_map, epoll_fd);
                    continue;

                case SENDING_HEADERS:
                    set_conn_state(curr_conn, SENDING_HEADERS);
                    ret = build_and_send_headers(response, curr_fd, curr_conn->offset,
                                                 original_state == SENDING_HEADERS, RESPONSE);
                    if (ret < 0) {
                        fprintf(stderr, "Failed to send HTTP headers to client\n");
                        // Special case where we can't send an HTTP message to the client, so simply
                        // close the fd.
                        cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                        continue;
                    } else if (ret > 0) {
                        curr_conn->offset = ret; // build_and_send_headers() returns the number of
                                                 // bytes read for a partial read
                        memset(&ev, 0, sizeof(struct epoll_event));
                        ev.events = SEND_EPOLL_FLAGS;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_fd, &ev) == -1) {
                            perror("epoll_ctl for client socket:");
                            cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                            continue;
                        }
                        fprintf(stderr, "[FD: %d] Sent back to epoll\n", curr_fd);
                        continue;
                    }
                    [[fallthrough]];
                case SENDING_BODY:
                    set_conn_state(curr_conn, SENDING_BODY);
                    int body_length = get_file_length(response->body_fd);
                    if (body_length > 0) {
                        ret = build_and_send_body(response, curr_fd);
                        if (ret < 0) {
                            fprintf(stderr, "Failed to send HTTP headers to client\n");
                            // Special case where we can't send an HTTP message to the client, so
                            // simply close the fd.
                            cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                            continue;
                        } else if (ret > 0) {
                            memset(&ev, 0, sizeof(struct epoll_event));
                            ev.events = SEND_EPOLL_FLAGS;
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_fd, &ev) == -1) {
                                perror("epoll_ctl for client socket:");
                                cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS,
                                                   epoll_fd);
                                continue;
                            }
                            fprintf(stderr, "[FD: %d] Sent back to epoll\n", curr_fd);
                            continue;
                        }
                    }
                    [[fallthrough]];
                default:
                    const char *connection_header_value
                        = get_header_value(curr_conn->request->headers,
                                           curr_conn->request->header_count, "Connection");
                    if (connection_header_value && strcmp(connection_header_value, "close") == 0) {
                        cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                        continue;
                    }

                    memset(&ev, 0, sizeof(struct epoll_event));
                    ev.events = RECV_EPOLL_FLAGS;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_fd, &ev) == -1) {
                        perror("epoll_ctl for client socket:");
                        cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                        continue;
                    }

                    set_conn_state(curr_conn, IDLE);
                }
            }

            // Read Operation
            else if (curr_event.events & EPOLLIN) {

                int ret = 0;
                int original_state = curr_conn->state;

                if (curr_conn->request == NULL) {
                    shallow_copy_http_message_to_conn(curr_conn, init_http_message(), REQUEST);
                }

                if (curr_conn->response == NULL) {
                    shallow_copy_http_message_to_conn(curr_conn, init_http_message(), RESPONSE);
                    add_header(curr_conn->response, "Server", SERVER_NAME);
                }

                HTTP_MESSAGE *response = curr_conn->response;
                HTTP_MESSAGE *request = curr_conn->request;

                switch (curr_conn->state) {
                case IDLE:
                    if (allocate_conn_buffer(curr_conn, 8 * KB) < 0) {
                        fprintf(stderr, "allocate_conn_buffer() error\n");
                        build_error_response(response, STATUS_INTERNAL_SERVER_ERROR,
                                             "Internal Server Error", NULL);
                        send_error_response(response, curr_fd, connection_map, epoll_fd);
                        continue;
                    } else {
                        // Reset the buffer on reading a new request
                        curr_conn->offset = 0;
                        memset(curr_conn->buffer, 0, 8 * KB);
                    }
                    [[fallthrough]];
                case PARSING_HEADERS:
                    set_conn_state(curr_conn, PARSING_HEADERS);
                    ret = parse_http_headers(request, curr_conn->buffer, 8 * KB, curr_fd,
                                             original_state == PARSING_HEADERS, REQUEST);
                    if (ret < 0) {
                        fprintf(stderr, "Failed to parse HTTP request headers\n");
                        build_error_response(response, STATUS_BAD_REQUEST, "Bad Request", NULL);
                        send_error_response(response, curr_fd, connection_map, epoll_fd);
                        continue;
                    } else if (ret > 0) {
                        // Send back to epoll
                        fprintf(stderr, "[FD: %d] Sent back to epoll\n", curr_fd);
                        continue;
                    }
                    [[fallthrough]];
                case PARSING_BODY:
                    set_conn_state(curr_conn, PARSING_BODY);
                    ret = parse_http_body(request, curr_conn->buffer, 8 * KB, curr_fd,
                                          original_state == PARSING_BODY);
                    if (ret < 0) {
                        fprintf(stderr, "Failed to parse HTTP request body\n");
                        build_error_response(response, STATUS_BAD_REQUEST, "Bad Request", NULL);
                        send_error_response(response, curr_fd, connection_map, epoll_fd);
                        continue;
                    } else if (ret > 0) {
                        // Send back to epoll
                        fprintf(stderr, "[FD: %d] Sent back to epoll\n", curr_fd);
                        continue;
                    }

                    // Print HTTP Request for DEBUG purposes
                    print_http_message(request, REQUEST);

                    if (server_router(request, response) != 0) {
                        fprintf(stderr, "Failed to parse HTTP request\n");
                        // error response is built inside the server router
                        send_error_response(response, curr_fd, connection_map, epoll_fd);
                        continue;
                    };

                    // Print the built HTTP Response for DEBUG purposes
                    print_http_message(response, RESPONSE);
                    [[fallthrough]];
                case SENDING_HEADERS:
                    set_conn_state(curr_conn, SENDING_HEADERS);
                    ret = build_and_send_headers(response, curr_fd, curr_conn->offset,
                                                 original_state == SENDING_HEADERS, RESPONSE);
                    if (ret < 0) {
                        fprintf(stderr, "Failed to send HTTP headers to client\n");
                        // Special case where we can't send an HTTP message to the client, so simply
                        // close the fd.
                        cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                        continue;
                    } else if (ret > 0) {
                        curr_conn->offset = ret; // build_and_send_headers() returns the number of
                                                 // bytes read for a partial read
                        memset(&ev, 0, sizeof(struct epoll_event));
                        ev.events = SEND_EPOLL_FLAGS;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_fd, &ev) == -1) {
                            perror("epoll_ctl for client socket:");
                            cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                            continue;
                        }
                        fprintf(stderr, "[FD: %d] Sent back to epoll\n", curr_fd);
                        continue;
                    }
                    [[fallthrough]];
                case SENDING_BODY:
                    set_conn_state(curr_conn, SENDING_BODY);
                    int body_length = get_file_length(response->body_fd);
                    if (body_length > 0) {
                        ret = build_and_send_body(response, curr_fd);
                        if (ret < 0) {
                            fprintf(stderr, "Failed to send HTTP headers to client\n");
                            // Special case where we can't send an HTTP message to the client, so
                            // simply close the fd.
                            cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                            continue;
                        } else if (ret > 0) {
                            memset(&ev, 0, sizeof(struct epoll_event));
                            ev.events = SEND_EPOLL_FLAGS;
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_fd, &ev) == -1) {
                                perror("epoll_ctl for client socket:");
                                cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS,
                                                   epoll_fd);
                                continue;
                            }
                            fprintf(stderr, "[FD: %d] Sent back to epoll\n", curr_fd);
                            continue;
                        }
                    }
                    [[fallthrough]];
                default:
                    const char *connection_header_value
                        = get_header_value(curr_conn->request->headers,
                                           curr_conn->request->header_count, "Connection");
                    if (connection_header_value && strcmp(connection_header_value, "close") == 0) {
                        fprintf(stderr, "[FD %d]: Closed connection\n", curr_fd);
                        cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                        continue;
                    }

                    memset(&ev, 0, sizeof(struct epoll_event));
                    ev.events = RECV_EPOLL_FLAGS;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_fd, &ev) == -1) {
                        perror("epoll_ctl for client socket:");
                        cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                        continue;
                    }

                    set_conn_state(curr_conn, IDLE);
                }
            }

            // Close client connection
            else if (curr_event.events & EPOLLRDHUP) {
                cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                continue;
            }

            else if (curr_event.events & (EPOLLERR | EPOLLHUP)) {
                int err = 0;
                socklen_t errlen = sizeof(err);
                if (getsockopt(curr_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
                    perror("getsockopt");
                } else if (err) {
                    fprintf(stderr, "socket error on fd %d: %s\n", curr_fd, strerror(err));
                } else {
                    fprintf(stderr, "hangup on fd %d\n", curr_fd);
                }
                cleanup_connection(connection_map, curr_fd, MAX_CONNECTIONS, epoll_fd);
                continue;
            }
        }

        // Check for current connection timeouts
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            time_t now = time(NULL);
            int ret;

            // Keep the server fd alive
            if (connection_map[i].fd == server_fd) {
                continue;
            }

            if (connection_map[i].fd == -1) {
                continue;
            }

            if (now - connection_map[i].last_activity > TIMEOUT_LIMIT) {

                if (connection_map[i].response == NULL) {
                    shallow_copy_http_message_to_conn(&connection_map[i], init_http_message(),
                                                      RESPONSE);
                }

                build_error_response(connection_map[i].response, STATUS_REQUEST_TIMEOUT,
                                     "Request Timeout", NULL);
                add_header(connection_map[i].response, "Server", SERVER_NAME);
                add_header(connection_map[i].response, "Connection", "close");

                ret = send_error_response(connection_map[i].response, connection_map[i].fd,
                                          connection_map, epoll_fd);
                if (ret < 0) {
                    fprintf(stderr, "Failed to send HTTP response to client successfully\n");
                }

                cleanup_connection(connection_map, connection_map[i].fd, MAX_CONNECTIONS, epoll_fd);
            }

            else if (connection_map[i].action_count >= ACTIONS_LIMIT) {

                if (connection_map[i].response == NULL) {
                    shallow_copy_http_message_to_conn(&connection_map[i], init_http_message(),
                                                      RESPONSE);
                }

                build_error_response(connection_map[i].response, STATUS_REQUEST_TIMEOUT,
                                     "Request Timeout", NULL);
                add_header(connection_map[i].response, "Server", SERVER_NAME);
                add_header(connection_map[i].response, "Connection", "close");

                ret = send_error_response(connection_map[i].response, connection_map[i].fd,
                                          connection_map, epoll_fd);
                if (ret < 0) {
                    fprintf(stderr, "Failed to send HTTP response to client successfully\n");
                }

                cleanup_connection(connection_map, connection_map[i].fd, MAX_CONNECTIONS, epoll_fd);
            }
        }
    }

    // Cleanup code
    free_conn_map(connection_map, MAX_CONNECTIONS);
    if (epoll_fd != -1)
        close(epoll_fd);
    if (server_fd != -1)
        close(server_fd);

    return 0;
}

int
main(void)
{
    return epoll_implementation();
}