#ifndef WEBSERVER_H
#define WEBSERVER_H
#include "helper.h"
#include "request.h"
#include "response.h"
#include "routing.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#define MAXCONN 8192
#define BUFF_SIZE 1024
static SOCKET server = INVALID_SOCKET;

int startServer(char *addr, int port);


#endif
