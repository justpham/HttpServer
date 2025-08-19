
#pragma once

#include "http_lib.h"

// HTTP struct helper functions
const char *read_crlf_line(const char *str, char *buffer, size_t bufsize);

// Parsing functions
int parse_start_line(char *line, HTTP_START_LINE *start_line, int http_message_type);

int parse_header(char *line, HTTP_HEADER *header);

int parse_body(const char *body_buffer, int content_length, FILE *fd);

int parse_body_stream(FILE *in_fd, int content_length, FILE *out_fd);