#include "main.h"
void indexH(Request *req, Response *res) {
  setBodyFromFile("./public/index.html", res);
}
int main(int argc, char const *argv[]) {

  int init = initializeSocket();
  if (init == -1) {
    cleanupRoutes();
    return -1;
  }
  char path[100];
  setPublicDir("./public");
  addRoute("/", indexH);
  startServer("0.0.0.0", 6969);
  cleanupRoutes();
}
