
#pragma once

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "http_lib.h"

// HTTP struct helper functions
const char *read_crlf_line(const char *str, char *buffer, size_t bufsize);

// Parsing functions
int parse_start_line(char *line, HTTP_START_LINE *start_line, int http_message_type);

int parse_header(char *line, HTTP_HEADER *header);

int parse_body(const char *body_buffer, int content_length, int fd);

int parse_body_stream(int in_fd, int content_length, char *buffer, int out_fd);

HTTP_MESSAGE
parse_http_message(int client_fd, int http_message_type);