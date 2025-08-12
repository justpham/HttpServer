/*
    Header File for socket and connect abstractions for a client
*/

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
#include "macros.h"
#include "ip_helper.h"

#define RECV_RATE           100     // max number of bytes we can get at once 

int connect_to_host (
    const char *hostname
);

int recieve_data(
    int sockfd, 
    char* buf, 
    int maxLength
);

