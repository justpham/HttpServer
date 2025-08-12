
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "macros.h"

#define RECV_RATE           100     // max number of bytes we can get at once 

// Function declarations
void *get_in_addr(
    struct sockaddr *sa
);

int connect_to_host (
    const char *hostname
);

void print_ip(
    char *message, 
    struct addrinfo *p, 
    bool stdout
);

int recieve_data(
    int sockfd, 
    char* buf, 
    int maxLength
);

