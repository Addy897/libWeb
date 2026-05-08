#ifndef REQUEST_H
#define REQUEST_H
#include "hash_table.h"

#ifdef _WIN32
#include <winsock2.h>
#elif defined(__linux__)
#include <sys/socket.h>
typedef int SOCKET;
#endif

#define MAX_CONTENT_SIZE (10 * 1024 * 1024)

typedef enum { GET = 0, POST = 1, HEAD = 2, OPTIONS = 3 } Method;
static const char *methods[] = {
    "GET",
    "POST",
    "HEAD",
    "OPTIONS",
};

#define METHODS_LEN sizeof(methods) / sizeof(methods[0])
typedef struct {
  Method method;
  char version[32];
  int body_len;
  char *path;
  char *body;
  HashTable *headers;
  HashTable *query_params;
} Request;

Request *initRequest();

const char *getParams(char *name, HashTable *params);
extern const char *getHeader(char *name, HashTable *headers);
void freeRequest(Request **req);
#endif
