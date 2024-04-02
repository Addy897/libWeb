#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <stdio.h>
#include <WinSock2.h>
#include <io.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include "parser.h" 
#include "request.h"
#include "helper.h"
#include "routing.h"
#include "httpResponse.h"
#define MAXCONN 100
#define BUFF_SIZE 1024
static SOCKET server=INVALID_SOCKET;
#endif