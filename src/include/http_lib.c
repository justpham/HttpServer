
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
print_http_message(const HTTP_MESSAGE *msg)
{
    if (!msg)
        return;

    printf("---------------HTTP MESSAGE---------------------\n");

    // Print start line
    if (msg->start_line.request.method[0] != '\0') {
        printf("HTTP Method: %s\n", msg->start_line.request.method);
        printf("Request Target: %s\n", msg->start_line.request.request_target);
        printf("HTTP Version: %s\n", msg->start_line.request.protocol);
    } else if (msg->start_line.response.protocol[0] != '\0') {
        printf("HTTP Protocol: %s\n", msg->start_line.response.protocol);
        printf("Status Code: %s\n", msg->start_line.response.status_code);
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
