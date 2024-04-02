#ifndef ROUTING_H
#define ROUTING_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"
#include "request.h"
#define MAX_PATH_LENGTH 100
#define HASH_SIZE 101 
#define MAX_ROUTES 100
typedef struct Route {
    char path[MAX_PATH_LENGTH];
    char* (*callback)(Request*);
    struct Route* next;
} Route;


unsigned int hash(char *str);
void addRoute(char * path,char*  (*callbackfunc)(Request*));
Route* hasRoute(char *path);
void cleanupRoutes();

#endif