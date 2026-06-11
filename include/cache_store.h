#ifndef CACHE_STORE_H
#define CACHE_STORE_H
#include "string_view.h"

#define STORE_MAX 16
typedef struct{
    char filepath[4096];
    StringView content;

}CacheItem;


typedef struct{
    CacheItem items[STORE_MAX];
    size_t i;
    size_t capacity;
}CacheStore;
CacheStore init_cache_store();
StringView get_content(char * filename,CacheStore * store);
void add_content(char * filename,StringView content,CacheStore* store);
void free_cache_items(CacheStore * store);


#endif
