#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include "hash_table.h"
#include "helper.h"
#include "mimeTypes.h"
#include "request.h"
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


Response *initResponse();
void setStatus(int status, Response *response);

void add_response_header(char *name, char *value, HashTable *headers);
void remove_response_header(char *name, HashTable *headers);

StringView get_response_header(char *name, HashTable *headers);
int get_all_response_headers(char **, HashTable *headers);
void setResponseBody(char *body, Response *);

void setBodyFromFile(char *pathname, Response *res);
char * responseToString(int *,Response*,Method);
void freeResponse(Response **response);
#endif
