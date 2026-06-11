#ifndef HASH_TABLE
#define HASH_TABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_view.h"
#define DEFAULT_SIZE 100
typedef struct Entry {
  StringView key;
  bool owns_key;
  void *value;
  struct Entry *next;
} HashEntry;
typedef struct {
  int capacity;
  int entry_count;
  HashEntry **entries;
} HashTable;
HashTable *init_table(int capacity);
void grow_table(HashTable *table);
const float load_factor(const HashTable *table);

#define get_entry(key,table,hashval) get_entry_sv(sv_from_cstr(key),table,hashval)
HashEntry *get_entry_sv(StringView sv, HashTable *table, unsigned int hashval);

#define get(key, table) get_from_sv(sv_from_cstr(key),table)
void * get_from_sv(StringView key,HashTable* table);


#define get_as_int(key,table) get_as_int_sv(sv_from_cstr(key),table)
int get_as_int_sv(StringView sv, HashTable *table);


#define get_as_sv_s(key,table) get_as_sv(sv_from_cstr(key),table)
StringView get_as_sv(StringView sv, HashTable *table);

#define get_as_cstr(key,table) get_as_cstr_sv(sv_from_cstr(key),table)
const char*  get_as_cstr_sv(StringView sv, HashTable *table);



#define get_as_float(key, table) get_as_float_sv(sv_from_cstr(key),table)
float get_as_float_sv(StringView sv, HashTable *table);


#define  add_with_deep_copy(key,value,val_size,table) add_sv(sv_from_cstr(key),value,val_size,table,true)
#define  add(key,value,val_size,table) add_sv(sv_from_cstr(key),value,val_size,table,false)
void add_sv(StringView sv, const void *value, int val_size, HashTable *table,bool deep_copy);

#define  remove_key(key,table) remove_key_sv(sv_from_cstr(key),table)
void remove_key_sv(StringView sv, HashTable *table);



void free_table(HashTable **hashtable);
#endif
