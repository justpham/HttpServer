
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_METHOD_LENGTH   10      // "OPTIONS" + null terminator
#define MAX_TARGET_LENGTH   2048    // Common URL length limit
#define MAX_VERSION_LENGTH  10      // "HTTP/1.1" + null terminator
#define MAX_LINE_LENGTH     2048    // Buffer size for reading HTTP lines
#define MAX_HEADERS         50      // Maximum number of header pairs
#define MAX_HEADER_LENGTH   256     // Maximum length per header line (reduced from 1024)
#define MAX_BODY_LENGTH     8192    // 8KB - reduced from 1MB for stack safety

typedef struct {
    char key[MAX_HEADER_LENGTH];
    char value[MAX_HEADER_LENGTH];
} HTTP_HEADER;

typedef struct {
    char method[MAX_METHOD_LENGTH];
    char request_target[MAX_TARGET_LENGTH];
    char http_version[MAX_VERSION_LENGTH];
    HTTP_HEADER headers[MAX_HEADERS];
    int header_count;
    char body[MAX_BODY_LENGTH];
    int body_length;
    int content_length;  // Parsed from Content-Length header
} HTTP_REQUEST;

// Function declarations
int parse_request(const char* raw_request, HTTP_REQUEST* request);
void print_request(const HTTP_REQUEST* request);
const char* get_header_value(const HTTP_REQUEST* request, const char* key);
const char* read_line(const char* str, char* buffer, size_t bufsize);