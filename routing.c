#include "include/routing.h"
#include "include/request.h"
#include <stdio.h>

HashTable *routes = NULL;
void initRoutes() {
  if (routes == NULL)
    routes = init_table(MAX_ROUTES);
}
Route *hasRoute(Method method, StringView path) {
  if (routes == NULL)
    return NULL;
  char key[1024];
  snprintf(key, 1024, "%s "SV_Fmt"", methods[method],SV_Arg(path) );

  Route *current_route = (Route *)get(key, routes);
  return current_route;
}
void addRoute(Method method, char *path,
              void (*callback)(Request *, Response *)) {
  if (routes == NULL) {
    initRoutes();
  }
  Route new_route = {.method = method, .callback = callback};
  char key[1024];
  snprintf(key, 1024, "%s %s", methods[method], path);
  add(key, &new_route, sizeof(new_route), routes);
}

void cleanupRoutes() {
  if (routes != NULL)
    free_table(&routes);
}
