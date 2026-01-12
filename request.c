#include "include/request.h"
#include "include/hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

Request *initRequest() {
  Request *req = calloc(1, sizeof(Request));
  req->body = nullptr;
  req->headers = nullptr;
  req->query_params = nullptr;
  req->path = nullptr;
  return req;
}
const char *getParams(char *name, HashTable *params) {
  return getHeader(name, params);
}
int setRequestHeaders(SOCKET client, Request *req) {
  if (req == NULL || client == 0)
    return 0;
  char buf[1024];
  int bytes_read = recv(client, buf, sizeof(buf), 0);
  if (bytes_read < 0)
    return 0;
  char *line;
  char *saveptr;
  char *token;
  char *headers;
  char delim[] = "\r\n\r\n";
  char *mid = strstr(buf, delim);
  if (!mid)
    return 0;

  char *body = mid + sizeof(delim) - 1;

  mid[1] = '\0';
  line = strtok_r(buf, "\n", &saveptr);
  token = strtok(line, " ");
  if (token == NULL)
    return 0;
  if (strcmp(token, "POST") == 0) {
    req->method = POST;
  } else {
    req->method = GET;
  }
  token = strtok(NULL, " ");
  if (token == NULL)
    return 0;
  char *version = strtok(NULL, " ");
  if (version == NULL)
    return 0;
  strcpy(req->version, version);
  char *query_start = strchr(token, '?');
  if (query_start != NULL) {
    req->query_params = initTable(16);
    char *path = strtok(token, "?");
    if (!path)
      return 0;
    req->path = strdup(path);
    char *query;
    while ((query = strtok_r(query_start, "&", &query_start)) != NULL) {
      char *key = strtok(query, "=");
      char *value = strtok(NULL, "=");
      if (!key || !value)
        return 0;
      add(key, value, strlen(value), req->query_params);
    }
  } else {
    req->path = strdup(token);
  }
  req->headers = initTable(16);
  while ((line = strtok_r(saveptr, "\n", &saveptr)) != NULL) {
    char *key = strtok(line, ":");
    char *value = strtok(NULL, ":");
    if (!key || !value)
      return 0;
    add(key, value, strlen(value), req->query_params);
  }
  if (*body != '\0') {
    req->body_len = strlen(body);
    req->body = strdup(body);
  }
  return 1;
}
int setRequestBody(SOCKET c, Request *req, int total_size) {
  char body[1024];
  while (req->body_len < total_size) {
    int bytes_read = recv(c, body, 1024, 0);
    if (bytes_read < 0)
      return 0;
    if (!req->body) {
      req->body = strdup(body);
      req->body_len = 0;
    } else {
      req->body = realloc(req->body, req->body_len + bytes_read);
      if (!req->body)
        return 0;
    }
    req->body_len += bytes_read;
  }
  return 1;
}

int buildRequest(SOCKET c, Request *req) {
  if (!setRequestHeaders(c, req)) {
    return 0;
  }
  const char *content_string = getAsString("Content-Length", req->headers);
  if (content_string) {
    int content_size = atoi(content_string);
    return setRequestBody(c, req, content_size);
  }
  return 1;
}

void freeRequest(Request **req) {
  if (!req || !*req)
    return;
  free((*req)->path);
  free((*req)->body);
  freeTable(&(*req)->headers);
  freeTable(&(*req)->query_params);
  free(*req);
  req = NULL;
}
