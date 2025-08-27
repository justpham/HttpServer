

#define _GNU_SOURCE

#include "http_parser.h"

const char *
read_crlf_line(const char *str, char *buffer, size_t bufsize)
{
    if (!str || !*str)
        return NULL;
    size_t i = 0;

    // Look for CRLF sequence only
    while (str[i] && i < bufsize - 1) {
        // Check if we found CRLF (make sure both characters exist)
        if (str[i] == '\r' && str[i + 1] && str[i + 1] == '\n') {
            break;
        }
        buffer[i] = str[i];
        i++;
    }
    buffer[i] = '\0';

    // Skip past CRLF if found and both characters are available
    if (str[i] == '\r' && str[i + 1] && str[i + 1] == '\n') {
        return str + i + 2; // Skip both \r and \n
    }

    // If we reached end of string or buffer without finding complete CRLF, return NULL
    return NULL;
}

/*
    Parses the HTTP start line given for a given line
*/
int
parse_start_line(char *line, HTTP_START_LINE *start_line, int http_message_type)
{
    int num_parsed = -1;
    char buffer[sizeof(HTTP_START_LINE)]; // Initialize a buffer to parse any line in the header

    if (!line || !*line) {
        fprintf(stderr, "Invalid line input to parse_start_line()\n");
        return -1;
    }

    if (!start_line) {
        fprintf(stderr, "Invalid start_line pointer in parse_start_line()\n");
        return -2;
    }

    // Read the line
    read_crlf_line(line, buffer, sizeof(buffer));

    // Empty line indicates end of headers
    if (strlen(buffer) == 0) {
        fprintf(stderr, "Empty header line encountered\n");
        return -3;
    }

    if (http_message_type == REQUEST) {

        char method_buffer[MAX_METHOD_LENGTH] = { 0 };
        char protocol_buffer[MAX_PROTOCOL_LENGTH] = { 0 };

        num_parsed = sscanf(buffer, "%9s %2047s %9s", method_buffer,
                            start_line->request.request_target, protocol_buffer);
        if (num_parsed == 3) {
            set_http_method_from_string(method_buffer, &start_line->request.method);
            set_http_protocol_from_string(protocol_buffer, &start_line->request.protocol);
        }
    } else if (http_message_type == RESPONSE) {
        char protocol_buffer[MAX_PROTOCOL_LENGTH] = { 0 };
        char status_code_buffer[MAX_STATUS_CODE_LENGTH] = { 0 };

        num_parsed = sscanf(buffer, "%9s %3s %2047[^\r\n]", protocol_buffer, status_code_buffer,
                            start_line->response.status_message);

        if (num_parsed == 3) {
            set_http_protocol_from_string(protocol_buffer, &start_line->response.protocol);
            set_http_status_code_from_string(status_code_buffer, &start_line->response.status_code);
        }

    } else {
        fprintf(stderr, "Unknown HTTP message type in parse_start_line()\n");
        return -4;
    }

    if (num_parsed == 3) {
        return 0;
    }

    fprintf(stderr, "Failed to parse start line: '%s'\n", buffer);
    return -5;
}

/*
    Parses the HTTP header given for a given line
*/
int
parse_header(char *line, HTTP_HEADER *header)
{
    char buffer[sizeof(HTTP_HEADER)]; // Initialize a buffer to parse any line in the header

    if (!line || !*line) {
        fprintf(stderr, "Invalid line input to parse_header()\n");
        return -1;
    }

    if (!header) {
        fprintf(stderr, "Invalid header pointer in parse_header()\n");
        return -2;
    }

    // Read the line
    read_crlf_line(line, buffer, sizeof(buffer));

    // Empty line indicates end of headers
    if (strlen(buffer) == 0) {
        fprintf(stderr, "Empty header line encountered\n");
        return -3;
    }

    // Use proper buffer size limits in sscanf to prevent overflow
    char temp_key[MAX_HEADER_LENGTH] = { 0 };
    char temp_value[MAX_HEADER_LENGTH] = { 0 };

    int num_parsed = sscanf(buffer, "%8000[^:]: %8000[^\r\n]", temp_key, temp_value);

    if (num_parsed == 2) {
        strncpy(header->key, temp_key, MAX_HEADER_LENGTH - 1);
        header->key[MAX_HEADER_LENGTH - 1] = '\0';
        strncpy(header->value, temp_value, MAX_HEADER_LENGTH - 1);
        header->value[MAX_HEADER_LENGTH - 1] = '\0';
    } else {
        fprintf(stderr, "Failed to parse header line: '%s'\n", buffer);
        return -4;
    }

    return 0;
}

/*
    Parses the HTTP body to output to a file descriptor

    This function assumes that out_fd is valid
*/
int
parse_body(const char *body_buffer, int content_length, int out_fd)
{
    if (!body_buffer || content_length < 0 || out_fd < 0) {
        fprintf(stderr, "Invalid arguments to parse_body()\n");
        return -1;
    }

    if (content_length == 0)
        return 0;

    const char *p = body_buffer;
    int remaining = content_length;
    const size_t CHUNK = 8192;

    while (remaining > 0) {
        size_t to_write = remaining > (int) CHUNK ? CHUNK : (size_t) remaining;
        ssize_t written = write(out_fd, p, to_write);
        if (written == -1) {
            perror("write to fd failed");
            return -2;
        } else if (written == 0) {
            /* Nothing written but no error: avoid infinite loop */
            break;
        }
        p += written;
        remaining -= (int) written;
    }

    if (fsync(out_fd) != 0) {
        perror("fsync");
        return -3;
    }

    return remaining == 0 ? 0 : -4;
}

/*
    Streams the HTTP body from an input socket fd to an output file descriptor

    This function assumes that in_fd (socket descriptor) and out_fd (file descriptor) are valid
*/
int
parse_body_stream(int in_fd, int content_length, char *buffer, int out_fd)
{
    if (in_fd < 0 || out_fd < 0 || content_length < 0) {
        fprintf(stderr, "Invalid arguments to parse_body_stream()\n");
        return -1;
    }

    if (content_length == 0)
        return 0;

    int remaining = content_length;

    if (buffer && strlen(buffer) > 0) {
        // Write the remaining contents of the current buffer to the file
        size_t written = write(out_fd, buffer, strlen(buffer));
        if (written != strlen(buffer)) {
            perror("write failed");
            return -4;
        }
        remaining -= written;
    }

    while (remaining > 0) {
        size_t to_read = remaining > (int) sizeof(buffer) ? sizeof(buffer) : (size_t) remaining;
        ssize_t r = recv(in_fd, buffer, to_read, 0);

        // TODO : Add a time out system

        if (r == 0) {
            fprintf(stderr, "Unexpected EOF while reading body\n");
            return -2;
        }

        if (r == -1) {
            perror("read failed");
            return -3;
        }

        size_t written = write(out_fd, buffer, r);
        if ((ssize_t) written != r) {
            perror("write failed");
            return -4;
        }

        remaining -= (int) r;
    }

    if (fsync(out_fd) != 0) {
        perror("fsync");
        return -5;
    }

    return 0;
}

HTTP_MESSAGE
parse_http_message(int client_fd, int http_message_type)
{

    // Recieve HTTP Request
    char buffer[8 * KB] = { 0 };

    char *recv_start = &buffer[0]; // Point at the start of the array
    int recv_offset = 0;

    ssize_t bytes_received;
    int is_start_line = 1;
    int return_code = 0;
    int end_of_header = 0;
    int current_buffer_length = 0;

    HTTP_MESSAGE request = init_http_message();

    // Parse the initial header section of the HTTP request
    while ((bytes_received = recv(client_fd, recv_start, sizeof(buffer) - recv_offset - 1, 0))
           > 0) {

        bytes_received += recv_offset;

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        recv_offset = 0;
        recv_start = &buffer[recv_offset];

        // TODO: Add a timeout system

        if (bytes_received == -1) {
            perror("recv failed");
            break;
        }

        if (request.header_count >= MAX_HEADERS) {
            fprintf(stderr, "Maximum header count exceeded\n");
            break;
        }

        while (bytes_received > 0) {

            char *crlf = strstr(buffer, "\r\n");

            // If we cannot find the CRLF at the end of the buffer, receive more data
            if (!crlf) {

                recv_offset = bytes_received;
                recv_start = &buffer[recv_offset];

                memset(recv_start, 0, sizeof(buffer) - recv_offset);

                break;
            }

            if (strlen(buffer) >= 2 && strncmp(buffer, "\r\n", 2) == 0) {

                crlf += 1;    // Move to '\n' of CRLF
                *crlf = '\0'; // Null-terminate the line
                current_buffer_length = strlen(buffer);

                memmove(buffer, crlf + 1, bytes_received - current_buffer_length);
                bytes_received -= current_buffer_length;

                end_of_header = 1;

                break;
            }

            // Recieve the current crlf line from the buffer
            crlf += 1;    // Move to '\n' of CRLF
            *crlf = '\0'; // Null-terminate the line
            current_buffer_length = strlen(buffer);

            // Process the line (e.g., parse headers)
            if (is_start_line) {
                if (parse_start_line(buffer, &request.start_line, http_message_type) < 0) {
                    fprintf(stderr, "Failed to parse start line: '%s'\n", buffer);
                }
                is_start_line = 0;
            } else {
                if (parse_header(buffer, &request.headers[request.header_count++]) < 0) {
                    fprintf(stderr, "Failed to parse header: '%s'\n", buffer);
                }
            }

            // Remove the processed line from the buffer
            memmove(buffer, crlf + 1, bytes_received - current_buffer_length);
            bytes_received -= current_buffer_length + 1;
        }

        if (end_of_header) { // TODO: Refactor it so it's nicer
            break;
        }
    }

    const char *content_length
        = get_header_value(request.headers, request.header_count, "Content-Length");

    if (content_length) {
        request.body_length = atoi(content_length);
    } else {
        request.body_length = 0;
    }

    if (request.body_length > 0) {
        /*
        For every request we want to store the body in a temporary file

        Body may be too large to store in memory
        */

        // TODO: For multiple simultaneous requests, use a unique temp file for each request
        http_message_open_temp_file(&request, "http_body", request.body_length);

        // Parse the body
        if ((return_code
             = parse_body_stream(client_fd, request.body_length, buffer, request.body_fd))
            != 0) {
            fprintf(stderr, "Failed to parse body. Return code: %d\n", return_code);
        }
    }

    return request;
}