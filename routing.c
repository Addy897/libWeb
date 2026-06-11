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
    const char* method_str = methods[method];
    int m_len = strlen(method_str); 
    
    if (m_len + 1 + path.count >= sizeof(key)) return NULL;

    memcpy(key, method_str, m_len);
    key[m_len] = ' ';
    memcpy(key + m_len + 1, path.data, path.count);
    key[m_len + 1 + path.count] = '\0';

    return (Route *)get_from_sv(sv_from_size(key, m_len + 1 + path.count), routes);  
}
void addRoute(Method method, char *path,
              void (*callback)(Request *, Response *)) {
  if (routes == NULL) {
    initRoutes();
  }
  Route new_route = {.method = method, .callback = callback};
  char key[1024];
  snprintf(key, 1024, "%s %s", methods[method], path);
  add_with_deep_copy(key, &new_route, sizeof(new_route), routes);
}

void cleanupRoutes() {
  if (routes != NULL)
    free_table(&routes);
}
