
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_METHOD_LENGTH   10          // "OPTIONS" + null terminator
#define MAX_TARGET_LENGTH   2048        // Common URL length limit
#define MAX_VERSION_LENGTH  10          // "HTTP/1.1" + null terminator

#define MAX_PROTOCOL_LENGTH  10         // "HTTP/1.1" + null terminator
#define MAX_STATUS_CODE_LENGTH  4       // "200" + null terminator
#define MAX_STATUS_MESSAGE_LENGTH  32   // "OK" + null terminator

#define MAX_LINE_LENGTH     2048        // Buffer size for reading HTTP lines
#define MAX_HEADERS         50          // Maximum number of header pairs
#define MAX_HEADER_LENGTH   256         // Maximum length per header line

/*
    HTTP Request Start Line Struct

    Holds the parsed details of an HTTP request start line.
    Includes method, request target, and HTTP version.
*/
typedef struct {
    char method[MAX_METHOD_LENGTH];         // HTTP method (e.g., GET, POST)
    char request_target[MAX_TARGET_LENGTH]; // Request target (e.g., /index.html)
    char http_version[MAX_VERSION_LENGTH];  // HTTP version (e.g., HTTP/1.1)
} HTTP_REQUEST_START_LINE;

/*
    HTTP Response Start Line Struct

    Holds the parsed details of an HTTP response start line.
    Includes protocol, status code, and status message.
*/
typedef struct {
    char protocol[MAX_PROTOCOL_LENGTH]; // Protocol (e.g., HTTP/1.1)
    char status_code[MAX_STATUS_CODE_LENGTH]; // Status code (e.g., 200)
    char status_message[MAX_STATUS_MESSAGE_LENGTH]; // Status message (e.g., OK)
    char _padding[(MAX_METHOD_LENGTH + MAX_TARGET_LENGTH + MAX_VERSION_LENGTH) - (MAX_PROTOCOL_LENGTH + MAX_STATUS_CODE_LENGTH + MAX_STATUS_MESSAGE_LENGTH)]; // Padding for size match
} HTTP_RESPONSE_START_LINE;

/*
    HTTP Start Line Struct

    Holds the start line of an HTTP request or response.
*/
typedef union {
    HTTP_REQUEST_START_LINE request;
    HTTP_RESPONSE_START_LINE response;
} HTTP_START_LINE;

/*
    HTTP Header Struct

    Holds one entry of an HTTP Header
*/
typedef struct {
    char key[MAX_HEADER_LENGTH];
    char value[MAX_HEADER_LENGTH];
} HTTP_HEADER;

// HTTP struct helper functions
const char* read_crlf_line(
    const char* str, 
    char* buffer, 
    size_t bufsize
);

// Library functions
const char* get_header_value(
    const HTTP_HEADER** header_array,
    const int length,
    const char* key
);

// Parsing functions
int parse_start_line (
    char* line,
    HTTP_START_LINE* start_line
);

int parse_header (
    char* line,
    HTTP_HEADER* header
);

