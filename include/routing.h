#ifndef ROUTING_H
#define ROUTING_H
#include "hash_table.h"
#include "helper.h"
#include "request.h"
#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PATH_LENGTH 100
#define HASH_SIZE 101
#define MAX_ROUTES 100
typedef struct Route {
  Method method;
  void (*callback)(Request *, Response *);
} Route;

void initRoutes();
void addRoute(Method method, char *path,
              void (*callbackfunc)(Request *, Response *));
Route *hasRoute(Method method, char *path);
void cleanupRoutes();

#endif
