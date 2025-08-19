

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

    if (http_message_type == REQUEST)
        num_parsed = sscanf(buffer, "%9s %2047s %9s", start_line->request.method,
                            start_line->request.request_target, start_line->request.http_version);

    else if (http_message_type == RESPONSE)
        num_parsed = sscanf(buffer, "%9s %3s %31[^\r\n]", start_line->response.protocol,
                            start_line->response.status_code, start_line->response.status_message);

    else {
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
*/
int
parse_body(const char *body_buffer, int content_length, FILE *out_fd)
{
    if (!body_buffer || content_length < 0 || !out_fd) {
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
        size_t written = fwrite(p, 1, to_write, out_fd);
        if (written == 0) {
            if (ferror(out_fd)) {
                perror("write to fd failed");
                return -2;
            }
            /* Nothing written but no error: avoid infinite loop */
            break;
        }
        p += written;
        remaining -= (int) written;
    }

    if (fflush(out_fd) != 0) {
        perror("fflush");
        return -3;
    }

    return remaining == 0 ? 0 : -4;
}

// Stream body from an input FILE* (e.g. socket via fdopen) to output FILE*
int
parse_body_stream(FILE *in_fd, int content_length, FILE *out_fd)
{
    if (!in_fd || !out_fd || content_length < 0) {
        fprintf(stderr, "Invalid arguments to parse_body_stream()\n");
        return -1;
    }

    if (content_length == 0)
        return 0;

    char buffer[8192];
    int remaining = content_length;

    while (remaining > 0) {
        size_t to_read = remaining > (int) sizeof(buffer) ? sizeof(buffer) : (size_t) remaining;
        size_t r = fread(buffer, 1, to_read, in_fd);
        if (r == 0) {
            if (feof(in_fd)) {
                fprintf(stderr, "Unexpected EOF while reading body\n");
                return -2;
            }
            if (ferror(in_fd)) {
                perror("read failed");
                return -3;
            }
        }

        size_t written = fwrite(buffer, 1, r, out_fd);
        if (written != r) {
            perror("write failed");
            return -4;
        }

        remaining -= (int) r;
    }

    if (fflush(out_fd) != 0) {
        perror("fflush");
        return -5;
    }

    return 0;
}