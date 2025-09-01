#include "http_builder.h"

/*
    Builds all headers + start line for an HTTP message into a buffer
    and sends it through the socket file descriptor

    Flow:
        1. Build Start Line + Headers into a buffer
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

    char buf[MAX_HEADERS_SIZE] = { 0 }; // Buffer to write to the socket

    if (msg->body_fd != -1) {
        // Add Content-Length header
        char content_length_str[32];
        int file_length = get_file_length(msg->body_fd);
        char *path = msg->body_path;

        snprintf(content_length_str, sizeof(content_length_str), "%d", file_length);
        add_header(msg, "Content-Length", content_length_str);

        // Add Content-Type header
        char mime_type_buffer[MAX_HEADER_LENGTH] = { 0 };
        if (get_mime_type_from_path(path, mime_type_buffer, sizeof(mime_type_buffer)) == 0) {
            char content_type_header[MAX_HEADER_LENGTH] = { 0 };
            if (strncmp(mime_type_buffer, "text/", 5) == 0
                || strcmp(mime_type_buffer, "application/json") == 0
                || strcmp(mime_type_buffer, "application/xml") == 0) {
                // Manually construct the content type with charset to avoid truncation warning
                size_t mime_len = strlen(mime_type_buffer);
                if (mime_len + 15 < sizeof(content_type_header)) { // 15 = strlen("; charset=utf-8")
                    strcpy(content_type_header, mime_type_buffer);
                    strcat(content_type_header, "; charset=utf-8");
                } else {
                    // Fall back to just the MIME type if adding charset would truncate
                    strncpy(content_type_header, mime_type_buffer, sizeof(content_type_header) - 1);
                    content_type_header[sizeof(content_type_header) - 1] = '\0';
                }
            } else {
                strncpy(content_type_header, mime_type_buffer, sizeof(content_type_header) - 1);
                content_type_header[sizeof(content_type_header) - 1] = '\0';
            }
            add_header(msg, "Content-Type", content_type_header);
        } else {
            fprintf(stderr, "Failed to get MIME type\n");
            return -5;
        }
    }

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
        if (msg->start_line.request.method >= HTTP_METHOD_UNKNOWN
            || msg->start_line.request.request_target[0] == '\0'
            || msg->start_line.request.protocol >= HTTP_PROTOCOL_UNKNOWN) {
            return -3;
        }
    } else if (http_message_type == RESPONSE) {
        if (msg->start_line.response.protocol >= HTTP_PROTOCOL_UNKNOWN
            || msg->start_line.response.status_code == HTTP_STATUS_CODE_UNKNOWN
            || msg->start_line.response.status_message[0] == '\0') {
            return -3;
        }
    } else {
        return -4; // Unknown HTTP message type
    }

    if (http_message_type == REQUEST) {

        char method_buffer[MAX_METHOD_LENGTH] = { 0 };
        char protocol_buffer[MAX_PROTOCOL_LENGTH] = { 0 };

        get_value_from_http_method(msg->start_line.request.method, method_buffer,
                                   sizeof(method_buffer));
        get_value_from_http_protocol(msg->start_line.request.protocol, protocol_buffer,
                                     sizeof(protocol_buffer));

        snprintf(ptr, bytes_remaining, "%s %s %s\r\n", method_buffer,
                 msg->start_line.request.request_target, protocol_buffer);
    } else {

        char status_code_buffer[MAX_STATUS_CODE_LENGTH] = { 0 };
        char protocol_buffer[MAX_PROTOCOL_LENGTH] = { 0 };

        get_value_from_http_protocol(msg->start_line.response.protocol, protocol_buffer,
                                     sizeof(protocol_buffer));
        get_value_from_http_status_code(msg->start_line.response.status_code, status_code_buffer,
                                        sizeof(status_code_buffer));

        snprintf(ptr, bytes_remaining, "%s %s %s\r\n", protocol_buffer, status_code_buffer,
                 msg->start_line.response.status_message);
    }

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
    char read_buf[FILE_READ_BUFFER_SIZE] = { 0 };

    // Check if msg->body_fd is valid
    if (msg->body_fd == -1) {
        return -1;
    }

    while (bytes_remaining > 0) {

        lseek(msg->body_fd, 0, SEEK_SET);

        int bytes_to_read = MIN(bytes_remaining, FILE_READ_BUFFER_SIZE);

        int bytes_read = read(msg->body_fd, read_buf, bytes_to_read);
        if (bytes_read == -1) {
            perror("read failed");
            return -2;
        } else if (bytes_read == 0) {
            // Nothing has been read so nothing to actually send
            break;
        }

        int bytes_sent = send(sock_fd, read_buf, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("send failed");
            return -3;
        } else if (bytes_sent == 0) {
            // Nothing has been sent so nothing to actually read
            break;
        }

        bytes_remaining -= bytes_sent;
    }

    return 0;
}
