#ifndef HASH_TABLE
#define HASH_TABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEFAULT_SIZE 100
typedef struct Entry {
  char *key;
  void *value;
  struct Entry *next;
} HashEntry;
typedef struct {
  int capacity;
  int entry_count;
  HashEntry **entries;

} HashTable;
HashTable *initTable(int capacity);
void growTable(HashTable *table);
const float loadFactor(const HashTable *table);

HashEntry *getEntry(char *key, HashTable *table, unsigned int hashval);
void *get(char *key, HashTable *table);
int getAsInt(char *key, HashTable *table);
const char *getAsString(char *key, HashTable *table);
float getAsFloat(char *key, HashTable *table);
void add(char *key, const void *value, int val_size, HashTable *table);
void removeKey(char *key, HashTable *table);
void freeTable(HashTable **hashtable);
#endif
