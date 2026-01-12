#ifndef WEBSERVER_H
#define WEBSERVER_H
#include "helper.h"
#include "request.h"
#include "response.h"
#include "routing.h"
#include <WinSock2.h>
#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#define MAXCONN 100
#define BUFF_SIZE 1024
static SOCKET server = INVALID_SOCKET;
#endif
