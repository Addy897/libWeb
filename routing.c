#include "include/routing.h"

HashTable *routes = NULL;
void initRoutes() {
  if (routes == NULL)
    routes = initTable(MAX_ROUTES);
}
Route *hasRoute(char *path) {
  if (routes == NULL)
    return NULL;
  Route *current_route = (Route *)get(path, routes);
  return current_route;
}
void addRoute(char *path, void (*callback)(Request *, Response *)) {
  if (routes == NULL) {
    initRoutes();
  }
  Route new_route = {.callback = callback};
  if (strlen(path) < MAX_PATH_LENGTH - 1) {
    strcpy(new_route.path, path);
  }
  add(path, &new_route, sizeof(new_route), routes);
}

void cleanupRoutes() {
  if (routes != NULL)
    freeTable(&routes);
}
