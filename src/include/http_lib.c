
#include "http_lib.h"

/*
    Gets the value of a header by key (case-insensitive)
*/
const char *
get_header_value(const HTTP_HEADER **header_array, const int length, const char *key)
{
    if (!header_array || !key)
        return NULL;

    for (int i = 0; i < length; i++) {
        if (strcasecmp(header_array[i]->key, key) == 0) {
            return header_array[i]->value;
        }
    }
    return NULL;
}

void
init_http_message(HTTP_MESSAGE *msg)
{
    if (!msg)
        return;
    // Zero out header and start line
    memset(msg, 0, sizeof(*msg));
    msg->header_count = 0;
    msg->body_fd = NULL;
    memset(msg->body_path, 0, sizeof(msg->body_path));
    msg->body_length = 0;
}

void
free_http_message(HTTP_MESSAGE *msg)
{
    if (!msg)
        return;

    // Close body fd if open
    if (msg->body_fd) {
        fclose(msg->body_fd);
        msg->body_fd = NULL;
    }

    // Zero remaining fields for safety
    msg->body_length = 0;
    msg->header_count = 0;
    memset(msg->body_path, 0, sizeof(msg->body_path));

    // Note: we do not touch headers/start_line memory beyond zeroing count; caller
    // can reuse or free the struct as needed.
}

int
http_message_set_body_fd(HTTP_MESSAGE *msg, FILE *fd, const char *path, int length)
{
    if (!msg)
        return -1;

    // Close any previously-open fd/path
    if (msg->body_fd) {
        fclose(msg->body_fd);
        msg->body_fd = NULL;
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
