
#pragma once

#include <time.h>
#include <stdlib.h>
#include "http_lib.h"

enum CONN_STATE {
    IDLE,
    PARSING_HEADERS,
    PARSING_BODY,
    SENDING_HEADERS,
    SENDING_BODY,
    INACTIVE,
};

struct conn {
    int fd;                     // A socket FD
    int state;
    HTTP_MESSAGE *request;
    HTTP_MESSAGE *response;
    char *buffer;               // Any stored buffer
    int offset;
    int action_count;
    time_t last_activity;
};

// TODO: Add Buffer length parameter

// A connection map should be allocated on the stack
int initialize_conn_map(struct conn* map, int length);
int free_conn_map(struct conn *map, int length);
int add_conn_to_map(struct conn *map, int fd, int length);
struct conn* get_conn(struct conn *map, int fd, int length);
int remove_conn_from_map(struct conn* map, int fd, int length);
int get_conn_map_length(struct conn *map, int length);

int free_conn(struct conn *conn);
int shallow_copy_http_message_to_conn(struct conn *conn, HTTP_MESSAGE message, int http_message_type);
int set_conn_state(struct conn *conn, int conn_state);
int allocate_conn_buffer(struct conn *conn, int char_length);
int update_conn_time(struct conn *conn);
