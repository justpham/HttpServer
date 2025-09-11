
#define _GNU_SOURCE

#include "routes.h"

int
default_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{
    if (!request || !response) {
        build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", NULL);
        return -1;
    }

    response->start_line.response.status_code = STATUS_OK;
    strcpy(response->start_line.response.status_message, "OK");
    return http_message_open_existing_file(response, "html/index.html", O_RDONLY, false);
}

int
echo_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{

    if (!request || !response) {
        build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", NULL);
        return -1;
    }

    // Read the request body
    const char *content_type
        = get_header_value(request->headers, MAX_HEADER_LENGTH, "Content-Type");

    if (strcmp("text/plain", content_type) == 0) {

        ssize_t bytes_read = 0;

        char *file_contents = malloc(request->body_length + 1);

        if (!file_contents) {
            perror("Failed to allocate memory");
            return -1;
        }

        print_http_message(request, REQUEST);

        if ((bytes_read = read(request->body_fd, file_contents, request->body_length)) == -1) {
            perror("Failed to read from temp file");
            close(request->body_fd);
            free(file_contents);
            build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error",
                                 NULL);
            return -1;
        }

        // Pass it to the response
        add_header(response, "Content-Type", content_type);

        http_message_open_temp_file(response, bytes_read);

        if (write(response->body_fd, file_contents, bytes_read) == -1) {
            perror("Failed to write to temp file");
            close(response->body_fd);
            free(file_contents);
            build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error",
                                 NULL);
            return -1;
        }

        response->start_line.response.status_code = STATUS_OK;
        strcpy(response->start_line.response.status_message, "OK");
        free(file_contents);

    } else {
        // Unsupported media type
        response->start_line.response.status_code = STATUS_UNSUPPORTED_MEDIA_TYPE;
        strcpy(response->start_line.response.status_message, "Unsupported Media Type");
        http_message_open_existing_file(response, "html/UnsupportedMediaType.html", O_RDONLY,
                                        false);
        return -1;
    }

    return 0;
}

int
static_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{
    if (!request || !response) {
        build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", NULL);
        return -1;
    }

    add_header(response, "Connection", "close");

    printf("Static file request: %s\n", request->start_line.request.request_target);

    // Get route
    char route[MAX_TARGET_LENGTH + 1] = { 0 }; // +1 for the leading '.'
    size_t target_len = strlen(request->start_line.request.request_target);
    if (target_len >= MAX_TARGET_LENGTH) {
        printf("Request target too long: %s\n", request->start_line.request.request_target);
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return -1;
    }
    snprintf(route, sizeof route, ".%s", request->start_line.request.request_target);

    int method = request->start_line.request.method;

    response->start_line.response.protocol = request->start_line.request.protocol;

    // Get the absolute path of the requested file
    char resolved_path[MAX_TARGET_LENGTH];
    if (realpath(route, resolved_path) == NULL) {
        printf("Failed to resolve path: %s\n", route);
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return -1;
    }

    // Get the absolute path of the static directory
    char static_dir_resolved[MAX_TARGET_LENGTH];
    if (realpath(STATIC_PATH_STR, static_dir_resolved) == NULL) {
        printf("Failed to resolve static directory path: %s\n", STATIC_PATH_STR);
        response->start_line.response.status_code = STATUS_FORBIDDEN;
        strcpy(response->start_line.response.status_message, "Forbidden");
        http_message_open_existing_file(response, "html/Forbidden.html", O_RDONLY, false);
        return -1;
    }

    // Check that its a file rather than a directory
    struct stat path_stat;
    if (stat(resolved_path, &path_stat) != 0) {
        printf("Failed to stat path: %s\n", resolved_path);
        response->start_line.response.status_code = STATUS_NOT_FOUND;
        strcpy(response->start_line.response.status_message, "Not Found");
        http_message_open_existing_file(response, "html/NotFound.html", O_RDONLY, false);
        return -1;
    }
    if (S_ISDIR(path_stat.st_mode)) {
        printf("Requested path is a directory: %s\n", resolved_path);
        response->start_line.response.status_code = STATUS_FORBIDDEN;
    }

    // Check if the resolved path is still under the static directory
    if (strncmp(resolved_path, static_dir_resolved, strlen(static_dir_resolved)) != 0) {
        printf("Access denied: %s\n", resolved_path);
        response->start_line.response.status_code = STATUS_FORBIDDEN;
        strcpy(response->start_line.response.status_message, "Forbidden");
        http_message_open_existing_file(response, "html/Forbidden.html", O_RDONLY, false);
        return -1;
    }

    // Only accept GET requests
    if (method != HTTP_GET) {
        printf("Method not allowed: %d\n", method);
        response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
        strcpy(response->start_line.response.status_message, "Method Not Allowed");
        add_header(response, "Allow", "GET");
        return -1;
    }

    // File is accessible
    response->start_line.response.status_code = STATUS_OK;
    strcpy(response->start_line.response.status_message, "OK");
    http_message_open_existing_file(response, resolved_path, O_RDONLY, true);
    return 0;
}

int
favicon_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response)
{
    if (!request || !response) {
        build_error_response(response, STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", NULL);
        return -1;
    }

    int method = request->start_line.request.method;

    // Only accept GET requests
    if (method != HTTP_GET) {
        response->start_line.response.status_code = STATUS_METHOD_NOT_ALLOWED;
        strcpy(response->start_line.response.status_message, "Method Not Allowed");
        add_header(response, "Allow", "GET");
        return -1;
    }

    // Try to serve favicon.ico from static directory
    const char *favicon_path = "./static/favicon.ico";

    // Check if favicon exists
    if (access(favicon_path, F_OK) == 0) {
        // File exists, serve it
        response->start_line.response.status_code = STATUS_OK;
        strcpy(response->start_line.response.status_message, "OK");
        add_header(response, "Content-Type", "image/x-icon");
        add_header(response, "Cache-Control", "public, max-age=86400"); // Cache for 1 day
        return http_message_open_existing_file(response, favicon_path, O_RDONLY, true);
    } else {
        response->start_line.response.status_code = STATUS_NO_CONTENT;
        strcpy(response->start_line.response.status_message, "No Content");
        add_header(response, "Cache-Control", "public, max-age=86400");
        return 0;
    }
}
