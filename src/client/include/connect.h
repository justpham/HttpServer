/*
    Header File for socket and connect abstractions for a client
*/

#pragma once

#include "ip_helper.h"
#include "macros.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RECV_RATE 100 // max number of bytes we can get at once

int connect_to_host(const char *hostname);

int receive_data(int sockfd, char *buf, int maxLength);
