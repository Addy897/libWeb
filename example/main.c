#include <stdio.h>
#include "main.h"
void indexH(Request *req, Response *res) {
  setBodyFromFile("./public/index.html", res);
}
void indexPost(Request *req, Response *res) {
  if (req->body_len > 0) {
    setResponseBody(req->body, res);
  } else {
    setResponseBody("{\"result\":\"Sucess\"}", res);
  }
  add_response_header("content-type", "application/json", res->headers);
  StringView ct = get_response_header("content-type",res->headers);
  if(!sv_eq(ct,SV_NULL))
    printf("CT: "SV_Fmt"\n",SV_Arg(ct));
  
}
int main(int argc, char const *argv[]) {

 // int init = initializeSocket();
 // if (init == -1) {
 //   cleanupRoutes();
 //   return -1;
 // }
  char path[100];
  setPublicDir("./public");
  addRoute(GET, "/", indexH);
  addRoute(POST, "/", indexPost);
  startServer("0.0.0.0", 6969);
  cleanupRoutes();
}
