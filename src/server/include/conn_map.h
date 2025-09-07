
#pragma once

#include <time.h>

struct conn {
    int fd;
    time_t last_activity; 
};

int initialize_conn_map(struct conn* map, int length);
int add_conn_to_map(struct conn* map, int fd, int length);
int update_conn_time(struct conn* map, int fd, int length);
int remove_conn_to_map(struct conn* map, int fd, int length);