
#include "http_lib.h"

/*
    Gets the value of a header by key (case-insensitive)
*/
const char *
get_header_value(const HTTP_HEADER *header_array, const int length, const char *key)
{
    if (!header_array || !key)
        return NULL;

    for (int i = 0; i < length; i++) {
        if (strcasecmp(header_array[i].key, key) == 0) {
            return header_array[i].value;
        }
    }
    return NULL;
}

/*
    Initializes an HTTP message

    Does not open a new file descriptor, user is responsible
*/
HTTP_MESSAGE
init_http_message()
{
    HTTP_MESSAGE msg = { 0 };

    msg.header_count = 0;
    msg.body_fd = -1;
    memset(msg.body_path, 0, sizeof(msg.body_path));
    msg.body_length = 0;

    return msg;
}

/*
    Frees the resources associated with an HTTP message

    Closes the body file descriptor if open
*/
void
free_http_message(HTTP_MESSAGE *msg)
{
    if (!msg)
        return;

    // Close body fd if open
    if (msg->body_fd != -1) {
        close(msg->body_fd);
        msg->body_fd = -1;
    }

    // Zero remaining fields for safety
    msg->body_length = 0;
    msg->header_count = 0;
    memset(msg->body_path, 0, sizeof(msg->body_path));

    // Note: we do not touch headers/start_line memory beyond zeroing count; caller
    // can reuse or free the struct as needed.
}

/*
    Closes the existing fd if open and sets the new fd/path

    Assumes that the fd is currently open
*/
int
http_message_set_body_fd(HTTP_MESSAGE *msg, int fd, const char *path, int length)
{
    if (!msg)
        return -1;

    /* Guard against NULL path before calling strlen() */
    if (path) {
        size_t pathlen = strlen(path);
        if (pathlen >= sizeof(msg->body_path)) {
            fprintf(stderr, "Provided path is too long for body_path buffer\n");
            return -2;
        }
    }

    // Close any previously-open fd/path
    if (msg->body_fd != -1) {
        close(msg->body_fd);
        msg->body_fd = -1;
    }

    memset(msg->body_path, 0, sizeof(msg->body_path));

    msg->body_fd = fd;
    msg->body_length = length;

    if (path) {
        /* Copy into the fixed-size body_path buffer. Truncate if necessary. */
        strncpy(msg->body_path, path, sizeof(msg->body_path) - 1);
        msg->body_path[sizeof(msg->body_path) - 1] = '\0';
    }

    return 0;
}

void
print_http_message(const HTTP_MESSAGE *msg, int http_message_type)
{
    if (!msg)
        return;

    printf("---------------HTTP MESSAGE---------------------\n");

    // Print start line
    if (http_message_type == REQUEST) {

        char method_buffer[MAX_METHOD_LENGTH] = { 0 };
        char protocol_buffer[MAX_PROTOCOL_LENGTH] = { 0 };

        get_value_from_http_method(msg->start_line.request.method, method_buffer, sizeof(method_buffer));
        get_value_from_http_protocol(msg->start_line.request.protocol, protocol_buffer, sizeof(protocol_buffer));

        printf("HTTP Method: %s\n", method_buffer);
        printf("Request Target: %s\n", msg->start_line.request.request_target);
        printf("HTTP Version: %s\n", protocol_buffer);

    } else if (http_message_type == RESPONSE) {

        char status_code_buffer[MAX_STATUS_CODE_LENGTH] = { 0 };
        char protocol_buffer[MAX_PROTOCOL_LENGTH] = { 0 };

        get_value_from_http_protocol(msg->start_line.response.protocol, protocol_buffer, sizeof(protocol_buffer));
        get_value_from_http_status_code(msg->start_line.response.status_code, status_code_buffer, sizeof(status_code_buffer));

        printf("HTTP Protocol: %s\n", protocol_buffer);
        printf("Status Code: %s\n", status_code_buffer);
        printf("Status Message: %s\n", msg->start_line.response.status_message);
    } else {
        printf("Unknown HTTP Message Type\n");
    }

    // Print headers
    printf("Headers:\n");
    for (int i = 0; i < msg->header_count; i++) {
        printf("  %s: %s\n", msg->headers[i].key, msg->headers[i].value);
    }

    // Print body information
    printf("Body Length: %d\n", msg->body_length);
    printf("Body Path: %s\n", msg->body_path);

    printf("Body: ");
    // Print the body content
    if (msg->body_length > 0) {
        // Read via the file descriptor (works even after unlink)
        if (msg->body_fd >= 0) {
            // Rewind to start of file
            if (lseek(msg->body_fd, 0, SEEK_SET) == -1) {
                perror("lseek");
            } else {
                // Read directly from fd using read() system call
                char body_buffer[8192];
                ssize_t bytes_read;
                while ((bytes_read = read(msg->body_fd, body_buffer, sizeof(body_buffer))) > 0) {
                    fwrite(body_buffer, 1, (size_t) bytes_read, stdout);
                }
                if (bytes_read == -1) {
                    perror("read body");
                }
            }
        } else {
            printf("(no body fd available)");
        }
    }

    printf("\n\n------------------------------------------------\n");
}

int get_value_from_http_protocol(uint32_t version, char *buffer, int buffer_length)
{
    if (!buffer || buffer_length <= 0)
        return -1;

    switch (version) {
    case HTTP_1_0:
        return snprintf(buffer, buffer_length, "HTTP/1.0");
    case HTTP_1_1:
        return snprintf(buffer, buffer_length, "HTTP/1.1");
    case HTTP_2_0:
        return snprintf(buffer, buffer_length, "HTTP/2.0");
    default:
        return snprintf(buffer, buffer_length, "HTTP/Unknown");
    }
}

int get_value_from_http_method(uint32_t method, char *buffer, int buffer_length)
{
    if (!buffer || buffer_length <= 0)
        return -1;

    switch (method) {
    case HTTP_GET:
        return snprintf(buffer, buffer_length, "GET");
    case HTTP_POST:
        return snprintf(buffer, buffer_length, "POST");
    case HTTP_PUT:
        return snprintf(buffer, buffer_length, "PUT");
    case HTTP_DELETE:
        return snprintf(buffer, buffer_length, "DELETE");
    default:
        return snprintf(buffer, buffer_length, "UNKNOWN");
    }
}

int get_value_from_http_status_code(uint32_t status_code, char *buffer, int buffer_length)
{
    if (!buffer || buffer_length <= 0)
        return -1;

    switch (status_code) {
    case STATUS_OK:
        return snprintf(buffer, buffer_length, "200");
    case STATUS_BAD_REQUEST:
        return snprintf(buffer, buffer_length, "400");
    case STATUS_NOT_FOUND:
        return snprintf(buffer, buffer_length, "404");
    case STATUS_METHOD_NOT_ALLOWED:
        return snprintf(buffer, buffer_length, "405");
    default:
        return snprintf(buffer, buffer_length, "UNKNOWN");
    }
}

int set_http_protocol_from_string(const char *str, uint32_t *version)
{
    if (!str || !version)
        return -1;

    if (strcmp(str, "HTTP/1.0") == 0) {
        *version = HTTP_1_0;
    } else if (strcmp(str, "HTTP/1.1") == 0) {
        *version = HTTP_1_1;
    } else if (strcmp(str, "HTTP/2.0") == 0) {
        *version = HTTP_2_0;
    } else {
        *version = HTTP_PROTOCOL_UNKNOWN;
    }

    return 0;
}

int set_http_method_from_string(const char *str, uint32_t *method)
{
    if (!str || !method)
        return -1;

    if (strcmp(str, "GET") == 0) {
        *method = HTTP_GET;
    } else if (strcmp(str, "POST") == 0) {
        *method = HTTP_POST;
    } else if (strcmp(str, "PUT") == 0) {
        *method = HTTP_PUT;
    } else if (strcmp(str, "DELETE") == 0) {
        *method = HTTP_DELETE;
    } else {
        *method = HTTP_METHOD_UNKNOWN;
    }

    return 0;
}

int set_http_status_code_from_string(const char *str, uint32_t *status_code)
{
    if (!str || !status_code)
        return -1;

    if (strcmp(str, "200") == 0) {
        *status_code = STATUS_OK;
    } else if (strcmp(str, "400") == 0) {
        *status_code = STATUS_BAD_REQUEST;
    } else if (strcmp(str, "404") == 0) {
        *status_code = STATUS_NOT_FOUND;
    } else if (strcmp(str, "405") == 0) {
        *status_code = STATUS_METHOD_NOT_ALLOWED;
    } else {
        *status_code = -1; // Unknown status code
    }

    return 0;
}
