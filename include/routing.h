#ifndef ROUTING_H
#define ROUTING_H
#include "hash_table.h"
#include "helper.h"
#include "httpResponse.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PATH_LENGTH 100
#define HASH_SIZE 101
#define MAX_ROUTES 100
typedef struct Route {
  char path[MAX_PATH_LENGTH];
  void (*callback)(Request *, Response *);
} Route;

void initRoutes();
void addRoute(char *path, void (*callbackfunc)(Request *, Response *));
Route *hasRoute(char *path);
void cleanupRoutes();

#endif
