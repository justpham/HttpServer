
#include "conn_map.h"

#define _GNU_SOURCE

int 
initialize_conn_map(struct conn *map, int length)
{
    for (int i = 0; i < length; i++) {
        map[i].fd = -1;
        map[i].offset = 0;
        map[i].action_count = 0;
        map[i].state = INACTIVE;
        map[i].buffer = NULL;
        map[i].request = NULL;
        map[i].response = NULL;
        map[i].last_activity = -1;
    }

    return 0;
}

int
free_conn_map(struct conn *map, int length)
{
    for (int i = 0; i < length; i++) {
        free_conn(&map[i]);
    }

    return 0;
}

int
add_conn_to_map(struct conn *map, int fd, int length)
{

    if (fd == -1) {
        fprintf(stderr, "Cannot add fd = -1 to map\n");
    }

    for (int i = 0; i < length; i++) {
        if (map[i].fd == -1) {
            map[i].fd = fd;
            map[i].offset = 0;
            map[i].action_count = 0;
            map[i].state = IDLE;
            map[i].buffer = NULL;
            map[i].request = NULL;
            map[i].response = NULL;
            map[i].last_activity = time(NULL);
            return 0;
        }
    }

    return -1;
}

struct conn*
get_conn(struct conn *map, int fd, int length)
{

    if (fd == -1) {
        return NULL;
    }

    for (int i = 0; i < length; i++) {
        if (map[i].fd == fd) {
            return &map[i];
        }
    }

    return NULL;
}

int
remove_conn_from_map(struct conn *map, int fd, int length)
{
    if (fd == -1) {
        return -1;
    }

    for (int i = 0; i < length; i++) {
        if (map[i].fd == fd) {
            return free_conn(&map[i]);
        }
    }

    return -1;
}

int
get_conn_map_length(struct conn *map, int length)
{
    int count = 0;

    for (int i = 0; i < length; i++) {
        if (map[i].fd != -1) {
            count++;
        }
    }

    return count;
}

int
free_conn(struct conn *conn)
{

    if (conn == NULL) {
        return -1;
    }

    if (conn->fd != -1) {
        close(conn->fd);
    }

    conn->fd = -1;
    conn->offset = 0;
    conn->action_count = -1;
    conn->state = INACTIVE;
    conn->last_activity = -1;

    if (conn->buffer != NULL) {
        free(conn->buffer);
    }

    if (conn->request != NULL) {
        free_http_message(conn->request);
        free(conn->request);
    }

    if (conn->response != NULL) {
        free_http_message(conn->response);
        free(conn->response);
    }

    return 0;
}

int
set_conn_state(struct conn *conn, int conn_state)
{
    if (conn == NULL) {
        return -1;
    }

    char *state = "UNKNOWN";

    switch (conn_state) {
        case IDLE:
            state = "IDLE";
            break;
        case PARSING_HEADERS:
            state = "PARSING_HEADERS";
            break;
        case PARSING_BODY:
            state = "PARSING_BODY";
            break;
        case SENDING_HEADERS:
            state = "SENDING_HEADERS";
            break;
        case SENDING_BODY:
            state = "SENDING_BODY";
            break;
        default:
            state = "UNKNOWN";
            break;
    }


    fprintf(stderr, "[FD: %d] Entering State %s\n", conn->fd, state);
    conn->state = conn_state;

    return 0;
}

/*
    Takes a HTTP_MESSAGE and allocates memory on the heap to store the message
*/
int
shallow_copy_http_message_to_conn(struct conn *conn, HTTP_MESSAGE message, int http_message_type)
{
    if (conn == NULL) {
        return -1;
    }

    if (http_message_type == REQUEST) {
        conn->request = calloc(1, sizeof(HTTP_MESSAGE));
        *conn->request = message;
    } else if (http_message_type == RESPONSE) {
        conn->response = calloc(1, sizeof(HTTP_MESSAGE));
        *conn->response = message;
    } else {
        return -1;
    }
    return 0;

}

int
allocate_conn_buffer(struct conn *conn, int char_length)
{
    if (conn == NULL) {
        return -1;
    }
    
    if (conn->buffer != NULL) {
        return 1;   // Not necessarily an error, but keep the old buffer
    }

    conn->offset = 0;
    conn->buffer = calloc(char_length, sizeof(char));

    return 0;
}

int
update_conn_time(struct conn *conn)
{
    if (conn == NULL) {
        return -1;
    }
    
    conn->last_activity = time(NULL);
    return 0;
}

