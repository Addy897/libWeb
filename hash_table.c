#include "include/hash_table.h"
static unsigned int hash(char *key, HashTable *table) {
  if (table == NULL)
    return -1;
  unsigned long hash = 5381;
  while (*key != '\0') {
    // hash = hash*33 + key[i]
    hash = ((hash << 5) + hash) + *key;
    key++;
  }
  return hash % table->capacity;
}

HashTable *initTable(int capacity) {
  HashTable *hashtable = malloc(sizeof(HashTable));
  if (hashtable == NULL)
    return NULL;
  if (capacity <= 0)
    capacity = DEFAULT_SIZE;
  hashtable->entries = calloc(capacity, sizeof(HashEntry *));
  hashtable->capacity = capacity;
  hashtable->entry_count = 0;
  return hashtable;
}
void growTable(HashTable *table) {

  table->capacity *= 2;
  HashEntry **new_entries = calloc(table->capacity, sizeof(HashEntry *));
  if (table->entries != NULL) {
    for (int i = 0; i < table->capacity / 2; i++) {
      HashEntry *current = table->entries[i];
      while (current != NULL) {
        HashEntry *temp = current;
        int hashval = hash(temp->key, table);
        current = current->next;
        temp->next = new_entries[hashval];
        new_entries[hashval] = temp;
      }
    }
  }
  free(table->entries);
  table->entries = new_entries;
}

const float loadFactor(const HashTable *table) {

  return (float)table->entry_count / (float)table->capacity;
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
      table->entry_count--;
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
    table->entry_count++;
    if (loadFactor(table) >= 0.7)
      growTable(table);
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
    for (int i = 0; i < (*hashtable)->capacity; i++) {
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
const char *getAsString(char *key, HashTable *table) {
  void *val = get(key, table);
  if (val == NULL)
    return 0;
  return (char *)val;
}
