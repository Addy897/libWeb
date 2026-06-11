#ifndef GLOBALS_H
#define GLOBALS_H
#include "hash_table.h"
#include "cache_store.h"
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


extern char PUBLIC_DIR[PATH_MAX];
extern size_t PUBLIC_DIR_LEN;
extern CacheStore FILE_CACHE;
#endif
