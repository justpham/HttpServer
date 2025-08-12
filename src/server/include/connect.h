/*
    Header File for socket and connect abstractions for a server
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "macros.h"
#include "ip_helper.h"

#define BACKLOG 10   // how many pending connections queue will hold

void sigchld_handler (
    int s
);

int server_setup (
    void
);

int accept_connection (
    int sockfd
);

void handle_client_connection (
    int sockfd,
    int client_fd,
    const char* message
);