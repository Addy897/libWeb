#include "request.h"
#include "response.h"
#include "helper.h"
#include "hash_table.h"
#include <stdlib.h>
#include <string.h>

Request *initRequest() {
  Request *req = calloc(1, sizeof(Request));
  req->body = SV_NULL;
  //req->headers = NULL;
  //req->query_params = NULL;
  req->path =SV_NULL;
  return req;
}
StringView get_request_header(char *name, Request *req) {
  StringView key = sv_from_cstr(name);
  for(int i = 0;i < req->headers.count;i++){
        Header header  = req->headers.items[i];
        if(sv_eq_ignorecase(key,header.key)){
            return header.value;
        }
  }
 return SV_NULL;
}
StringView get_request_header_sv(StringView key, Request *req) {
    for(int i = 0;i < req->headers.count;i++){
        Header header = req->headers.items[i];
        if(sv_eq(key,header.key)){
            return header.value;
        }
    }
    return SV_NULL;
    // return get_as_sv(name, req->headers);
}
void freeRequest(Request **req) {
  if (!req || !*req)
    return;
  //free_table(&(*req)->headers);
  //free_table(&(*req)->query_params);
  free(*req);
  *req = NULL;
}
