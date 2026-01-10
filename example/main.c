#include "main.h"
#include <stdlib.h>
#include <string.h>
char *indexH(Request *req, Response *res) {
  char content[] =
      "<html><head><title>C</title></head><body>Hello From C</body></html>";
  int n = strlen(content);
  char *htmlResp = malloc(n);
  strncpy(htmlResp, content, n);
  htmlResp[n] = '\0';
  return htmlResp;
}

int main(int argc, char const *argv[]) {

  int init = initializeSocket();
  if (init == -1) {
    cleanupRoutes();
    return -1;
  }
  char path[100];

  addRoute("/", indexH);
  startServer("0.0.0.0", 6969);
  cleanupRoutes();
}
