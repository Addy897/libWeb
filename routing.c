#include "routing.h"
Route* route_table[HASH_SIZE]={NULL};
unsigned int hash(char *str) {
    unsigned int hashval;
    for (hashval = 0; *str != '\0'; str++) {
        hashval = *str + 31 * hashval;
    }
    return hashval % HASH_SIZE;
}
Route* hasRoute(char *path){
    unsigned int hashval = hash(path);
        Route* current_route = route_table[hashval];
        while (current_route != NULL) {
            if (strcmp(current_route->path,path) == 0) {
                break;
            }
            current_route = current_route->next;
        }
        return current_route;
}
void addRoute(char *path, char* (*callback)(Request*)) {
    unsigned int hashval = hash(path);
    Route* new_route = (Route*)malloc(sizeof(Route));
    if (new_route == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    strcpy(new_route->path, path);
    new_route->callback = callback;
    new_route->next = route_table[hashval];
    route_table[hashval] = new_route;
}



void cleanupRoutes(){
    for (int i = 0; i < HASH_SIZE; i++) {
        Route* current = route_table[i];
        while (current != NULL) {
            Route* temp = current;
            current = current->next;
            free(temp);
        }
    }
}
