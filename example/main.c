#include <stdio.h>
#include "main.h"
void indexH(Request *req, Response *res) {
  setBodyFromFile("./public/index.html", res);
}
void indexPost(Request *req, Response *res) {
  if (req->body.count > 0) {
    set_response_body_sv(req->body, res);
  } else {
    set_response_body("{\"result\":\"Sucess\"}", res);
  }
  add_response_header("content-type", "application/json", res);
  StringView ct = get_response_header("content-type",res);
  
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
