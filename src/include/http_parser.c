

#define _GNU_SOURCE

#include "http_parser.h"

const char* read_line(const char* str, char* buffer, size_t bufsize) {
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
    Parses an HTML header
*/
int parse_request(
    const char* raw_request, 
    HTTP_REQUEST* request
) {

    if (!raw_request || !*raw_request) {
        fprintf(stderr, "raw_request in parse_request() is NULL!\n");
        return -1;
    }

    if (request == NULL) return -1;

    char line_buffer[MAX_LINE_LENGTH];
    const char *current_pos = raw_request;

    // Parse request line: <method> <request-target> <protocol>
    current_pos = read_line(current_pos, line_buffer, sizeof(line_buffer));
    if (!current_pos) {
        fprintf(stderr, "Failed to read request line\n");
        return -1;
    }

    sscanf(line_buffer, "%s %s %s", request->method, request->request_target, request->http_version);

    // Check if all three components were parsed
    if (strlen(request->method) == 0 || strlen(request->request_target) == 0 || strlen(request->http_version) == 0) {
        fprintf(stderr, "Malformed request line: missing method, target, or version\n");
        return -6;
    }

    // Validate HTTP Method
    const char* valid_methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"};
    int method_valid = 0;
    size_t num_methods = sizeof(valid_methods) / sizeof(valid_methods[0]);
    for (size_t i = 0; i < num_methods; i++) {
        if (strcmp(request->method, valid_methods[i]) == 0) {
            method_valid = 1;
            break;
        }
    }
    if (!method_valid) {
        fprintf(stderr, "Invalid HTTP method: %s\n", request->method);
        return -1;
    }
    
    // Validate request target
    if (strlen(request->request_target) == 0) {
        fprintf(stderr, "Empty request target\n");
        return -2;
    }
    
    // Check for origin form (starts with /) or absolute form (starts with http://)
    if (request->request_target[0] != '/' && strncmp(request->request_target, "http://", 7) != 0 && strncmp(request->request_target, "https://", 8) != 0) {
        fprintf(stderr, "Invalid request target format: %s (must be origin form starting with '/' or absolute form starting with 'http://' or 'https://')\n", request->request_target);
        return -3;
    }
    
    // Additional validation for origin form - ensure it doesn't contain spaces or control characters
    if (request->request_target[0] == '/') {
        for (int i = 0; request->request_target[i] != '\0'; i++) {
            char c = request->request_target[i];
            if (c < 32 || c == 127) {  // Control characters
                fprintf(stderr, "Invalid character in request target\n");
                return -4;
            }
        }
    }
    
    // Validate HTTP version (only allow HTTP/1.1)
    if (strcmp(request->http_version, "HTTP/1.1") != 0) {
        fprintf(stderr, "Unsupported HTTP version: %s (only HTTP/1.1 is supported)\n", request->http_version);
        return -5;
    }

    // Parse headers
    request->header_count = 0;
    while (current_pos && *current_pos && request->header_count < MAX_HEADERS) {
        current_pos = read_line(current_pos, line_buffer, sizeof(line_buffer));
        
        if (!current_pos) break; // If there is no more request to parse
        
        // Empty line indicates end of headers
        if (strlen(line_buffer) == 0) {
            break;
        }
        
        HTTP_HEADER *current_header = &request->headers[request->header_count];

        // Use proper buffer size limits in sscanf to prevent overflow
        char temp_key[MAX_HEADER_LENGTH];
        char temp_value[MAX_HEADER_LENGTH];
        
        int parsed = sscanf(line_buffer, "%1023[^:]: %1023[^\r\n]", temp_key, temp_value);
        
        if (parsed == 2) {
            strncpy(current_header->key, temp_key, MAX_HEADER_LENGTH - 1);
            current_header->key[MAX_HEADER_LENGTH - 1] = '\0';
            strncpy(current_header->value, temp_value, MAX_HEADER_LENGTH - 1);
            current_header->value[MAX_HEADER_LENGTH - 1] = '\0';
                        
            request->header_count++;
        } else {
            fprintf(stderr, "Failed to parse header line: '%s'\n", line_buffer);
        }
    }

    // Check if Host is present
    const char* host_str = get_header_value(request, "Host");
    if (!host_str) {
        fprintf(stderr, "Missing Host header\n");
        return -6;
    }

    // Extract Content-Length if present
    const char* content_length_str = get_header_value(request, "Content-Length");
    if (content_length_str) {
        request->content_length = atoi(content_length_str);
    } else {
        request->content_length = 0;
    }

    // Parse body (remaining content after headers)
    if (current_pos && *current_pos) {
        size_t remaining_length = strlen(current_pos);
        size_t copy_length = (remaining_length < MAX_BODY_LENGTH - 1) ? remaining_length : MAX_BODY_LENGTH - 1;
        strncpy(request->body, current_pos, copy_length);        
        request->body[copy_length] = '\0';
        request->body_length = copy_length;
    } else {
        request->body[0] = '\0';
        request->body_length = 0;
    }

    return 0;
}

/*
    Gets the value of a header by key (case-insensitive)
*/
const char* get_header_value(const HTTP_REQUEST* request, const char* key) {
    if (!request || !key) return NULL;
    
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].key, key) == 0) {
            return request->headers[i].value;
        }
    }
    return NULL;
}

/*
    Print out the HTTP_REQUEST struct
*/
void print_request(
    const HTTP_REQUEST* request
) {
    if (!request) return;

    printf("=== Parsed Request Details ===\n");
    printf("Method: %s\n", request->method);
    printf("Request Target: %s\n", request->request_target);
    printf("HTTP Version: %s\n", request->http_version);
    printf("Header Count: %d\n", request->header_count);
    printf("Content Length: %d\n", request->content_length);
    printf("Body Length: %d\n", request->body_length);
    printf("Body: %s\n", request->body);
    printf("==============================\n");
}
