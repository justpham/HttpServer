
#include "conn_map.h"

#define _GNU_SOURCE

int 
initialize_conn_map(struct conn* map, int length)
{
    for (int i = 0; i < length; i++) {
        map[i].fd = -1;
        map[i].last_activity = -1;
    }

    return 0;
}

int
add_conn_to_map(struct conn* map, int fd, int length)
{
    for (int i = 0; i < length; i++) {
        if (map[i].fd == -1) {
            map[i].fd = fd;
            map[i].last_activity = time(NULL);
            return 0;
        }
    }

    return -1;
}

int
update_conn_time(struct conn* map, int fd, int length)
{
    for (int i = 0; i < length; i++) {
        if (map[i].fd == fd) {
            map[i].last_activity = time(NULL);
            return 0;
        }
    }

    return -1;
}

int
remove_conn_to_map(struct conn* map, int fd, int length)
{
    for (int i = 0; i < length; i++) {
        if (map[i].fd == fd) {
            map[i].fd = -1;
            map[i].last_activity = -1;
            return 0;
        }
    }

    return -1;
}