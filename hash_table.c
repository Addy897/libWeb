#include "hash_table.h"
static unsigned int hash(const char *key, HashTable *table) {
  if (table == NULL)
    return -1;
  unsigned long hash = 5381;
  for (int i = 0; i < strlen(key); i++) {
    // hash = hash*33 + key[i]
    hash = ((hash << 5) + hash) + key[i];
  }
  return hash % table->size;
}

HashTable *initTable(int size) {
  HashTable *hashtable = malloc(sizeof(HashTable));
  if (hashtable == NULL)
    return NULL;
  hashtable->entries = calloc(size, sizeof(HashEntry *));
  hashtable->size = size;
  return hashtable;
}
HashEntry *getEntry(char *key, HashTable *table, unsigned int hashval) {
  if (table == NULL || table->entries == NULL)
    return NULL;
  HashEntry *entry = table->entries[hashval];
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      break;
    }
    entry = entry->next;
  }
  return entry;
}
void removeKey(char *key, HashTable *table) {
  if (table == NULL || table->entries == NULL)
    return;
  int hashval = hash(key, table);
  HashEntry *curr = table->entries[hashval];
  HashEntry *prev = NULL;

  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      if (prev == NULL) {
        table->entries[hashval] = curr->next;
      } else {
        prev->next = curr->next;
      }
      free(curr->key);
      free(curr->value);
      free(curr);
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}
void add(char *key, const void *value, int val_size, HashTable *table) {
  if (table == NULL || table->entries == NULL)
    return;

  unsigned int hashval = hash(key, table);
  HashEntry *entry = getEntry(key, table, hashval);
  if (entry == NULL) {
    entry = malloc(sizeof(HashEntry));
    entry->value = malloc(val_size);
    entry->key = strdup(key);
    entry->next = table->entries[hashval];
    table->entries[hashval] = entry;
  } else {
    free(entry->value);
    entry->value = malloc(val_size);
  }
  memcpy(entry->value, value, val_size);
}

void *get(char *key, HashTable *table) {
  if (table == NULL || table->entries == NULL)
    return NULL;

  int hashval = hash(key, table);
  HashEntry *entry = getEntry(key, table, hashval);
  if (entry == NULL)
    return NULL;
  return entry->value;
}
void freeTable(HashTable **hashtable) {
  if (!hashtable || !(*hashtable))
    return;
  if ((*hashtable)->entries != NULL) {
    for (int i = 0; i < (*hashtable)->size; i++) {
      HashEntry *current = (*hashtable)->entries[i];
      while (current != NULL) {
        HashEntry *temp = current;
        current = current->next;
        free(temp->key);
        free(temp->value);
        free(temp);
      }
    }
  }
  free((*hashtable)->entries);
  free(*hashtable);
  *hashtable = NULL;
}
int getAsInt(char *key, HashTable *table) {
  void *val = get(key, table);
  if (val == NULL)
    return 0;
  return *(int *)val;
}
float getAsFloat(char *key, HashTable *table) {
  void *val = get(key, table);
  if (val == NULL)
    return 0.0f;
  return *(float *)val;
}
char *getAsString(char *key, HashTable *table) {
  void *val = get(key, table);
  if (val == NULL)
    return 0;
  return (char *)val;
}
