#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include "helper.h"
#include "mimeTypes.h"
#include <WinSock2.h>
#include <fcntl.h>
#include <stdio.h>
typedef struct {
  int status;
  char *headers;
  char *body;
} Response;

void HTMLResponse(char *response, char *body);
void HTTPResponse(char *body, char *resp, int size, char *type);
char *render_template(char *pathname);
int sendFile(SOCKET client, char *filepath);
void initResponse(Response **response);
void addHeader(char *name, char *value, Response *response);
void setBody(char *body, Response *);
void freeResponse(Response **response);
#endif
