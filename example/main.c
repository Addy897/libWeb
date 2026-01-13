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
  addHeader("Content-Type", "application/json", res->headers);
}
int main(int argc, char const *argv[]) {

  int init = initializeSocket();
  if (init == -1) {
    cleanupRoutes();
    return -1;
  }
  char path[100];
  setPublicDir("./public");
  addRoute(GET, "/", indexH);
  addRoute(POST, "/", indexPost);
  startServer("0.0.0.0", 6969);
  cleanupRoutes();
}
