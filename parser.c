#include "include/parser.h"
#include <stdio.h>
void parseRequest(char *request, Request *req) {
    char *line;
    char *saveptr;
    char *token;
    char *body_start;
    line = strtok_r((char *)request, "\n", &saveptr);
    
    token = strtok(line, " ");
    
    strncpy(req->method, token,strlen(token));
    token = strtok(NULL, " ");
    char * version=strtok(NULL," ");
    strncpy(req->version,version,strlen(version));
    char *query_start = strchr(token, '?');
    if (query_start != NULL) {
        char * path=strtok(token,"?");
        strcpy(req->path, path);
        char * query = strtok(query_start + 1, "&");
        req->query_param_count = 0;
        while (query != NULL && req->query_param_count < 20) {
            strcpy(req->query_params[req->query_param_count++], query);
            query = strtok(NULL, "&");
        }
        *query_start = '\0';
    }else{
        req->query_param_count=0;
        strcpy(req->path, token);
        
        
    }
    req->header_count = 0;
    while ((line = strtok_r(NULL, "\n", &saveptr)) != NULL) {
        if (strlen(line) <= 2)
            break;
        strcpy(req->headers[req->header_count++], line);
    }
    
    body_start = strtok_r(NULL, "\n", &saveptr);
    if (body_start != NULL) {
        strcpy(req->body, body_start);
    } else {
        req->body[0] = '\0';
    }
}


