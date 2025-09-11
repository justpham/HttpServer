
#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_lib.h"

#define FILE_READ_BUFFER_SIZE 4 * KB

/*
    Builder functions to build an HTTP_MESSAGE to send to the socket
*/

int build_and_send_headers(HTTP_MESSAGE *msg, int sock_fd, int off, int continuing,
                           int http_message_type);
int build_header(HTTP_MESSAGE *msg, int http_message_type, char *buf, int buf_size);
int build_and_send_body(HTTP_MESSAGE *msg, int sock_fd);
