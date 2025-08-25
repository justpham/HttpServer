
#pragma once

#include "macros.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/***************************
 *
 * HTTP Definitions
 *
 ***************************/
#define MAX_METHOD_LENGTH 10     // "OPTIONS" + null terminator
#define MAX_TARGET_LENGTH 2 * KB // Common URL length limit
#define MAX_VERSION_LENGTH 10    // "HTTP/1.1" + null terminator

#define MAX_PROTOCOL_LENGTH 10       // "HTTP/1.1" + null terminator
#define MAX_STATUS_CODE_LENGTH 4     // "200" + null terminator
#define MAX_STATUS_MESSAGE_LENGTH 32 // "OK" + null terminator

#define MAX_LINE_LENGTH 2 * KB   // Buffer size for reading HTTP lines
#define MAX_HEADERS 50           // Maximum number of header pairs
#define MAX_HEADER_LENGTH 4 * KB // Maximum length per header line

#define HTTP_RESPONSE_START_LINE_PADDING                         \
    (MAX_METHOD_LENGTH + MAX_TARGET_LENGTH + MAX_VERSION_LENGTH) \
        - (MAX_PROTOCOL_LENGTH + MAX_STATUS_CODE_LENGTH + MAX_STATUS_MESSAGE_LENGTH)

#define MAX_HTTP_BODY_FILE_PATH 4 * KB

#define MAX_START_LINE_SIZE (MAX_METHOD_LENGTH + MAX_TARGET_LENGTH + MAX_VERSION_LENGTH) + 1
#define MAX_HEADER_SIZE (MAX_HEADERS * MAX_HEADER_LENGTH) + MAX_START_LINE_SIZE

enum HTTP_MESSAGE_TYPE
{
    REQUEST,
    RESPONSE
};

/*
    HTTP Request Start Line Struct

    Holds the parsed details of an HTTP request start line.
    Includes method, request target, and HTTP version.
*/
typedef struct
{
    char method[MAX_METHOD_LENGTH];         // HTTP method (e.g., GET, POST)
    char request_target[MAX_TARGET_LENGTH]; // Request target (e.g., /index.html)
    char protocol[MAX_VERSION_LENGTH];      // HTTP version (e.g., HTTP/1.1)
} HTTP_REQUEST_START_LINE;

/*
    HTTP Response Start Line Struct

    Holds the parsed details of an HTTP response start line.
    Includes protocol, status code, and status message.
*/
typedef struct
{
    char protocol[MAX_PROTOCOL_LENGTH];              // Protocol (e.g., HTTP/1.1)
    char status_code[MAX_STATUS_CODE_LENGTH];        // Status code (e.g., 200)
    char status_message[MAX_STATUS_MESSAGE_LENGTH];  // Status message (e.g., OK)
    char _padding[HTTP_RESPONSE_START_LINE_PADDING]; // Padding for size match
} HTTP_RESPONSE_START_LINE;

/*
    HTTP Start Line Struct

    Holds the start line of an HTTP request or response.
*/
typedef union
{
    HTTP_REQUEST_START_LINE request;
    HTTP_RESPONSE_START_LINE response;
} HTTP_START_LINE;

/*
    HTTP Header Struct

    Holds one entry of an HTTP Header
*/
typedef struct
{
    char key[MAX_HEADER_LENGTH];
    char value[MAX_HEADER_LENGTH];
} HTTP_HEADER;

typedef struct
{
    HTTP_START_LINE start_line;
    HTTP_HEADER headers[MAX_HEADERS];
    int header_count;
    int body_fd;                             // file descriptor for body contents
    char body_path[MAX_HTTP_BODY_FILE_PATH]; // optional file path
    int body_length;                         // length of body in bytes
} HTTP_MESSAGE;

/* HTTP_MESSAGE struct helper functions */
HTTP_MESSAGE init_http_message();
void free_http_message(HTTP_MESSAGE *msg);
int http_message_set_body_fd(HTTP_MESSAGE *msg, int fd, const char *path, int length);
void print_http_message(const HTTP_MESSAGE *msg);

// Library functions
const char *get_header_value(const HTTP_HEADER *header_array, const int length, const char *key);
