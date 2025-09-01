
#pragma once

#include "macros.h"
#include "random.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

/***************************
 *
 * HTTP Definitions
 *
 ***************************/
#define MAX_METHOD_LENGTH 10     // "OPTIONS" + null terminator
#define MAX_TARGET_LENGTH 2 * KB // Common URL length limit
#define MAX_VERSION_LENGTH 10    // "HTTP/1.1" + null terminator

#define MAX_PROTOCOL_LENGTH 10                      // "HTTP/1.1" + null terminator
#define MAX_STATUS_CODE_LENGTH 4                    // "200" + null terminator
#define MAX_STATUS_MESSAGE_LENGTH MAX_TARGET_LENGTH // "OK" + null terminator

#define MAX_LINE_LENGTH 2 * KB   // Buffer size for reading HTTP lines
#define MAX_HEADERS 50           // Maximum number of header pairs
#define MAX_HEADER_LENGTH 4 * KB // Maximum length per header line

#define MAX_HTTP_BODY_FILE_PATH 4 * KB

#define MAX_START_LINE_SIZE (MAX_METHOD_LENGTH + MAX_TARGET_LENGTH + MAX_VERSION_LENGTH) + 1

#define MAX_HEADERS_SIZE (MAX_HEADERS * MAX_HEADER_LENGTH) + MAX_START_LINE_SIZE

enum HTTP_MESSAGE_TYPE
{
    REQUEST,
    RESPONSE,
    HTTP_MESSAGE_TYPE_UNKNOWN
};

enum HTTP_STATUS_CODE
{
    STATUS_OK = 200,
    STATUS_BAD_REQUEST = 400,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_METHOD_NOT_ALLOWED = 405,
    STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
    STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_CODE_UNKNOWN = 999
};

enum HTTP_METHOD
{
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_METHOD_UNKNOWN
};

enum HTTP_PROTOCOL
{
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0,
    HTTP_PROTOCOL_UNKNOWN
};

/*
    HTTP Request Start Line Struct

    Holds the parsed details of an HTTP request start line.
    Includes method, request target, and HTTP version.
*/
typedef struct
{
    uint32_t protocol;                      // HTTP version (e.g., HTTP/1.1)
    uint32_t method;                        // HTTP method (e.g., GET, POST)
    char request_target[MAX_TARGET_LENGTH]; // Request target (e.g., /index.html)
} HTTP_REQUEST_START_LINE; // TODO : I can probably use integers and enums here to save space

/*
    HTTP Response Start Line Struct

    Holds the parsed details of an HTTP response start line.
    Includes protocol, status code, and status message.
*/
typedef struct
{
    uint32_t protocol;                              // Protocol (e.g., HTTP/1.1)
    uint32_t status_code;                           // Status code (e.g., 200)
    char status_message[MAX_STATUS_MESSAGE_LENGTH]; // Status message (e.g., OK)
} HTTP_RESPONSE_START_LINE; // TODO : I can probably use integers and enums here to save space

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
int get_file_length(int fd);
int add_header(HTTP_MESSAGE *msg, const char *key, const char *value);
int http_message_open_existing_file(HTTP_MESSAGE *msg, const char *path, int oflags,
                                    bool is_abspath);
int http_message_open_temp_file(HTTP_MESSAGE *msg, int body_length);
int http_message_set_body_fd(HTTP_MESSAGE *msg, int fd, const char *path, int body_length);
int build_error_response(HTTP_MESSAGE *msg, int status_code, const char *status_message,
                         const char *json_error_message);
void print_http_message(const HTTP_MESSAGE *msg, int http_message_type);

/* Enum Functions */
int get_value_from_http_protocol(uint32_t version, char *buffer, int buffer_length);
int get_value_from_http_method(uint32_t method, char *buffer, int buffer_length);
int get_value_from_http_status_code(uint32_t status_code, char *buffer, int buffer_length);
int set_http_protocol_from_string(const char *str, uint32_t *version);
int set_http_method_from_string(const char *str, uint32_t *method);
int set_http_status_code_from_string(const char *str, uint32_t *status_code);

/* HTTP_HEADER functions */
const char *get_header_value(const HTTP_HEADER *header_array, const int header_length,
                             const char *key);

/* Other helper functions*/
int get_mime_type_from_path(const char *path, char *buffer, int buffer_length);