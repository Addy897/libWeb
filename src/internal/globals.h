#ifndef GLOBALS_H
#define GLOBALS_H
#include "hash_table.h"
#include "cache_store.h"
#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

static const char *methods[] = {
    "GET",
    "POST",
    "HEAD",
    "OPTIONS",
};

#define METHODS_LEN sizeof(methods) / sizeof(methods[0])

extern char PUBLIC_DIR[PATH_MAX];
extern size_t PUBLIC_DIR_LEN;
extern __thread CacheStore FILE_CACHE;
#endif
