#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include "string_view.h"
#include "request.h"
#include <stdbool.h>

#include "request.h"
#include <fcntl.h>
#include <stdio.h>



typedef struct HashTable HashTable;

typedef struct {
  int code;
  const char *message;
} StatusCode;
typedef struct {
  StatusCode status;
  HashTable *headers;
  StringView body;
  bool owns_body;
} Response;



Response *initResponse();
void setStatus(int status, Response *response);

void add_response_header(char *name, char *value, Response *res);
void add_response_header_sv(StringView name, char *value, Response *res);
void remove_response_header(char *name, Response *res);
void remove_response_header_sv(StringView name, Response *res);

StringView get_response_header(char *name, Response *res);
StringView get_response_header_sv(StringView name, Response *res);
int get_all_response_headers(char **, Response *res);
void set_response_body(char *body, Response *);
void set_response_body_sv(StringView body, Response *);

void setBodyFromFile(char *pathname, Response *res);
char * responseToString(int *,Response*,Method);
void freeResponse(Response **response);
#endif
