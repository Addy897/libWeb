#ifndef WEBSERVER_H
#define WEBSERVER_H
#include "helper.h"
#include "request.h"
#include "response.h"
#include "routing.h"

#ifdef _WIN32
#include <io.h>
#elif defined(__linux__)
  #include <pthread.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  typedef void* LPVOID;
  #define closesocket close
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
  #define SOCKADDR        struct sockaddr
  #define SOCKADDR_IN     struct sockaddr_in
#endif

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#define MAXCONN 100
#define BUFF_SIZE 1024
static SOCKET server = INVALID_SOCKET;
#endif
