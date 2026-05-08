#include "include/request.h"
#include "include/response.h"
#include "include/helper.h"

#include "include/hash_table.h"
#include <stdlib.h>
#include <string.h>

Request *initRequest() {
  Request *req = calloc(1, sizeof(Request));
  req->body = NULL;
  req->headers = NULL;
  req->query_params = NULL;
  req->path = NULL;
  return req;
}
const char *getParams(char *name, HashTable *params) {
  return getHeader(name, params);
}

void freeRequest(Request **req) {
  if (!req || !*req)
    return;
  free((*req)->path);
  if((*req)->body !=NULL)
    free((*req)->body);
  freeTable(&(*req)->headers);
  freeTable(&(*req)->query_params);
  free(*req);
  req = NULL;
}
