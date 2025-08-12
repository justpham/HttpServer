
#pragma once

#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>

void *get_in_addr(
    struct sockaddr *sa
);

void print_ip(
    char *message,
    int ai_family,
    struct sockaddr *address, 
    bool stdout
);