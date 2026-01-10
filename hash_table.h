#ifndef HASH_TABLE
#define HASH_TABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Entry {
  char *key;
  void *value;
  struct Entry *next;
} HashEntry;
typedef struct {
  int size;
  HashEntry **entries;
} HashTable;
HashTable *initTable(int size);
HashEntry *getEntry(char *key, HashTable *table, unsigned int hashval);
void *get(char *key, HashTable *table);
int getAsInt(char *key, HashTable *table);
char *getAsString(char *key, HashTable *table);
float getAsFloat(char *key, HashTable *table);
void add(char *key, const void *value, int val_size, HashTable *table);
void removeKey(char *key, HashTable *table);
void freeTable(HashTable **hashtable);
#endif
