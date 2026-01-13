#ifndef REQUEST_H
#define REQUEST_H
#include "hash_table.h"
#include <winsock2.h>
typedef enum { GET = 0, POST = 1, HEAD = 2 } Method;
static const char *methods[] = {
    "GET",
    "POST",
    "HEAD",
};
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
int setRequestHeaders(SOCKET client, Request *req);
int setRequestBody(SOCKET c, Request *req, int total_size);

const char *getParams(char *name, HashTable *params);
extern const char *getHeader(char *name, HashTable *headers);
int buildRequest(SOCKET c, Request *req);
void freeRequest(Request **req);
#endif
