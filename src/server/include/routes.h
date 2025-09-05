
#pragma once

#include "http_builder.h"
#include "http_parser.h"
#include "connect.h"
#include "ip_helper.h"
#include "macros.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int default_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response);
int echo_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response);
int static_handler(HTTP_MESSAGE *request, HTTP_MESSAGE *response);
