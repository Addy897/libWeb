#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include "hash_table.h"
#include "helper.h"
#include "mimeTypes.h"
#include <WinSock2.h>
#include <fcntl.h>
#include <stdio.h>
typedef struct {
  int code;
  const char *message;
} StatusCode;
typedef struct {
  StatusCode status;
  HashTable *headers;
  char *body;
} Response;

static StatusCode StatusCodes[] = {{200, "OK"}, {404, "Not Found"}};
#define STATUS_COUNT (sizeof(StatusCodes) / sizeof(StatusCode))

int sendFile(SOCKET client, char *filepath);
int sendResponse(SOCKET *client, Response *response);

Response *initResponse();
void setStatus(int status, Response *response);
void addHeader(char *name, char *value, Response *response);
void removeHeader(char *name, Response *response);

const char *getHeader(char *name, Response *response);
char *getAllHeaders(Response *response);
void setBody(char *body, Response *);

void setBodyFromFile(char *pathname, Response *res);

void freeResponse(Response **response);
#endif
