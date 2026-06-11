#include "include/cache_store.h"
#include <string.h>
#include <stdlib.h>

CacheStore init_cache_store() {
    CacheStore store; 
    store.i = 0;
    store.capacity = STORE_MAX;
    memset(store.items, 0, sizeof(store.items)); 
    return store; }

StringView get_content(char * filename, CacheStore * store) {
    if (filename == NULL || store == NULL) return SV_NULL;
    
    for (int i = 0; i < store->i; i++) {
        CacheItem item = store->items[i];
        if (strcmp(filename, item.filepath) == 0) {
            return item.content;         
        }
    }
    return SV_NULL;
}

void add_content(char * filename, StringView content, CacheStore * store) {
    if (filename == NULL || store == NULL) return;
    if (store->i >= store->capacity) return; 
    
    CacheItem item; 
    
    strncpy(item.filepath, filename, sizeof(item.filepath) - 1);
    item.filepath[sizeof(item.filepath) - 1] = '\0'; 
    
    item.content = content;
    
    store->items[store->i] = item;
    store->i++;
}

void free_cache_items(CacheStore * store) {
    if (store == NULL) return;
    
    for (int i = 0; i < store->i; i++) {
        if (store->items[i].content.data != NULL) {
            free(store->items[i].content.data);
            store->items[i].content = SV_NULL;
        }
    }
    store->i = 0; 
}
