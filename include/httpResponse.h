#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <WinSock2.h>
#include <fcntl.h>

#include <stdio.h>
#include "mimeTypes.h"
#include "helper.h"
void HTMLResponse(char * response ,char * body);
void HTTPResponse(char * body,char *resp,int size,char* type);
char * render_template(char * pathname);
int sendFile(SOCKET client,char * filepath);
#endif