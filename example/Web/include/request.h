#ifndef REQUEST_H
#define REQUEST_H

#ifdef _WIN32
#include <winsock2.h>
#elif defined(__linux__)
#include <sys/socket.h>
typedef int SOCKET;
#endif


#include "string_view.h"
#include <stdbool.h>

typedef struct HashTable HashTable;


#define MAX_CONTENT_SIZE (10 * 1024 * 1024)

typedef enum { GET = 0, POST = 1, HEAD = 2, OPTIONS = 3 } Method;
typedef struct {
  Method method;
  StringView version;
  StringView path;
  StringView body;
  HashTable *headers;
  HashTable *query_params;
} Request;

Request *initRequest();

#define get_request_params(name, request) get_request_header(name, request)
StringView get_request_header(char *name, Request* req);
StringView get_request_header_sv(StringView name, Request * req);
void freeRequest(Request **req);
#endif
