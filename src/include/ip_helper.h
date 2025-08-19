
#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>

void *get_in_addr(struct sockaddr *sa);

void print_ip(char *message, int ai_family, struct sockaddr *address, bool stdout);
