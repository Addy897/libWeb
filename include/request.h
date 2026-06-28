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

#define MAX_HEADERS 32
#define MAX_CONTENT_SIZE (10 * 1024 * 1024)

typedef enum { GET = 0, POST = 1, HEAD = 2, OPTIONS = 3 } Method;

typedef struct {
    StringView key;
    StringView value;
}Header;

typedef struct {
    Header items[MAX_HEADERS];
    int count;
}HeadersList;



typedef struct {
  Method method;
  StringView version;
  StringView path;
  StringView body;
  HeadersList headers;
  HeadersList cookies;
  HeadersList query_params;
} Request;

Request *initRequest();

StringView get_request_param_sv(StringView name, Request *req);
StringView get_request_header(char *name, Request* req);
StringView get_request_header_sv(StringView name, Request * req);
StringView get_request_cookie_sv(StringView name, Request * req);
void freeRequest(Request **req);
#endif
