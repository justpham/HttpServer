#include "http_builder.h"

/*
    Builds all headers + start line for an HTTP message
    and sends it through the socket file descriptor

    Flow:
        1. Build Start Line + Headers
        2. Send To Socket

    Parameters:
        HTTP_MESSAGE msg - HTTP Message Data
        int sock_fd - Socket file descriptor
        int http_message_type - Type of HTTP message (REQUEST or RESPONSE)

    Returns:
        int - status code
*/
int
build_and_send_headers(HTTP_MESSAGE *msg, int sock_fd, int http_message_type)
{

    if (!msg || sock_fd == -1) {
        return -1;
    }

    char buf[MAX_HEADER_SIZE] = { 0 }; // Buffer to write to the socket

    if (build_header(msg, http_message_type, buf, sizeof(buf)) != 0) {
        return -2;
    }

    if (send(sock_fd, buf, strlen(buf), 0) == -1) {
        return -3;
    }

    return 0;
}

int
build_header(HTTP_MESSAGE *msg, int http_message_type, char *buf, int buf_size)
{
    char *ptr = buf;
    int bytes_remaining = buf_size;

    if (!msg) {
        return -1;
    }

    // Check parameters of msg->start_line
    if (http_message_type == REQUEST) {
        if (msg->start_line.request.method[0] == '\0'
            || msg->start_line.request.request_target[0] == '\0'
            || msg->start_line.request.protocol[0] == '\0') {
            return -3;
        }
    } else if (http_message_type == RESPONSE) {
        if (msg->start_line.response.protocol[0] == '\0'
            || msg->start_line.response.status_code[0] == '\0'
            || msg->start_line.response.status_message[0] == '\0') {
            return -3;
        }
    } else {
        return -4; // Unknown HTTP message type
    }

    snprintf(ptr, bytes_remaining, "%s %s %s\r\n",
             http_message_type == REQUEST ? msg->start_line.request.method
                                          : msg->start_line.response.protocol,
             http_message_type == REQUEST ? msg->start_line.request.request_target
                                          : msg->start_line.response.status_code,
             http_message_type == REQUEST ? msg->start_line.request.protocol
                                          : msg->start_line.response.status_message);

    bytes_remaining -= strlen(ptr);
    ptr += strlen(ptr);

    for (int i = 0; i < msg->header_count; i++) {

        if (bytes_remaining <= 0) {
            return -5; // Buffer overflow
        }

        snprintf(ptr, bytes_remaining, "%s: %s\r\n", msg->headers[i].key, msg->headers[i].value);
        bytes_remaining -= strlen(ptr);
        ptr += strlen(ptr);
    }

    if (bytes_remaining >= 3) {
        snprintf(ptr, bytes_remaining, "\r\n");
        bytes_remaining -= strlen(ptr);
        ptr += strlen(ptr);
    } else {
        return -6; // Not enough space for trailing CRLF
    }

    return 0;
}

int
build_and_send_message(HTTP_MESSAGE *msg, int sock_fd, int http_message_type)
{
    // Send headers
    build_and_send_headers(msg, sock_fd, http_message_type);

    // No body to send
    if (msg->body_length == 0) {
        return 0;
    }
    
    int bytes_remaining = msg->body_length;
    char read_buf[FILE_READ_BUFFER_SIZE] = {0};

    // Check if msg->body_fd is valid
    if (!msg->body_fd) {
        return -1;
    }

    while (bytes_remaining > 0) {

        lseek(msg->body_fd, 0, SEEK_SET);

        int bytes_to_read = min(bytes_remaining, FILE_READ_BUFFER_SIZE);

        int bytes_read = fread(read_buf, 1, bytes_to_read, msg->body_fd);
        if (bytes_read <= 0) {
            break;
        }

        int bytes_sent = send(sock_fd, read_buf, bytes_read, 0);
        if (bytes_sent <= 0) {
            break;
        }

        bytes_remaining -= bytes_sent;
    }

    return 0;
}
