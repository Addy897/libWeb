#include "request.h"
#include "response.h"
#include "helper.h"
#include "hash_table.h"
#include <stdlib.h>
#include <string.h>

Request *initRequest() {
  Request *req = calloc(1, sizeof(Request));
  req->body = SV_NULL;
  req->headers = NULL;
  req->query_params = NULL;
  req->path =SV_NULL;
  return req;
}
StringView get_request_header(char *name, Request *req) {
  return get_as_sv_s(name, req->headers);
}
StringView get_request_header_sv(StringView name, Request *req) {
    return get_as_sv(name, req->headers);
}
void freeRequest(Request **req) {
  if (!req || !*req)
    return;
  free_table(&(*req)->headers);
  free_table(&(*req)->query_params);
  free(*req);
  *req = NULL;
}
