

#define _GNU_SOURCE

#include "http_parser.h"

const char* read_crlf_line(const char* str, char* buffer, size_t bufsize) {
    if (!str || !*str) return NULL;
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
        return str + i + 2;  // Skip both \r and \n
    }
    
    // If we reached end of string or buffer without finding complete CRLF, return NULL
    return NULL;
}

/*
    Parses the HTTP start line given for a given line
*/
int parse_start_line (
    char* line,
    HTTP_START_LINE* start_line,
    int http_message_type
) {
    int num_parsed = -1;
    char buffer[sizeof(HTTP_START_LINE)];      // Initialize a buffer to parse any line in the header

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
        num_parsed = sscanf(buffer, "%s %s %s", 
            start_line->request.method, 
            start_line->request.request_target, 
            start_line->request.http_version
        );
        
    else if (http_message_type == RESPONSE) 
        num_parsed = sscanf(buffer, "%s %s %31[^\r\n]", 
            start_line->response.protocol, 
            start_line->response.status_code, 
            start_line->response.status_message
        );

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
int parse_header (
    char* line,
    HTTP_HEADER* header
) {
    char buffer[sizeof (HTTP_HEADER)];      // Initialize a buffer to parse any line in the header

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
    char temp_key[MAX_HEADER_LENGTH] = {0};
    char temp_value[MAX_HEADER_LENGTH] = {0};

    int num_parsed = sscanf(buffer, "%1023[^:]: %1023[^\r\n]", temp_key, temp_value);
    
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
    Gets the value of a header by key (case-insensitive)
*/
const char* get_header_value(
    const HTTP_HEADER** header_array,
    const int length,
    const char* key
) {
    if (!header_array || !key) return NULL;

    for (int i = 0; i < length; i++) {
        if (strcasecmp(header_array[i]->key, key) == 0) {
            return header_array[i]->value;
        }
    }
    return NULL;
}
