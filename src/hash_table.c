#include "hash_table.h"

static unsigned int hash_sv(StringView sv, HashTable *table) {
    if (table == NULL)
        return -1;
    unsigned long hash = 5381;
    int i = 0;
    while (i < sv.count) {
        unsigned char c = sv.data[i] | 0x20;
        // hash = hash*33 + c
        hash = ((hash << 5) + hash) + c;;
        i++;
    }
    return hash % table->capacity;
}
static unsigned int hash(char *key, HashTable *table) {
    return hash_sv(sv_from_cstr(key),table); 
}
HashTable *init_table(int capacity) {
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
void grow_table(HashTable *table) {

    int new_capacity = table->capacity * 2;

    HashEntry **new_entries = calloc(new_capacity, sizeof(HashEntry *));

    if (!new_entries)
        return;
    table->capacity = new_capacity;    
    if (table->entries != NULL) {
        for (int i = 0; i < table->capacity / 2; i++) {
            HashEntry *current = table->entries[i];
            while (current != NULL) {
                HashEntry *temp = current;
                int hashval = hash_sv(temp->key, table);
                current = current->next;
                temp->next = new_entries[hashval];
                new_entries[hashval] = temp;
            }
        }
    }
    free(table->entries);
    table->entries = new_entries;
}

const float load_factor(const HashTable *table) {

    return (float)table->entry_count / (float)table->capacity;
}

HashEntry *get_entry_sv(StringView sv, HashTable *table, unsigned int hashval) {
    if (table == NULL || table->entries == NULL)
        return NULL;
    HashEntry *entry = table->entries[hashval];
    while (entry != NULL) {
        if (sv_eq_ignorecase(entry->key,sv)) {
            break;
        }
        entry = entry->next;
    }
    return entry;
}


void remove_key_sv(StringView key, HashTable *table) {
    if (table == NULL || table->entries == NULL)
        return;
    int hashval = hash_sv(key, table);
    HashEntry *curr = table->entries[hashval];
    HashEntry *prev = NULL;

    while (curr != NULL) {
        if (sv_eq(curr->key, key)){
            if (prev == NULL) {
                table->entries[hashval] = curr->next;
            } else {
                prev->next = curr->next;
            }
            if(curr->owns_key)
                free(curr->key.data);
            curr->key.count = 0;
            curr->key.data = NULL;
            free(curr->value);
            free(curr);
            table->entry_count--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void add_sv(StringView sv, const void *value, int val_size, HashTable *table,bool deep_copy_key) {
    if (table == NULL || table->entries == NULL)
        return;
    if (table->entry_count * 10 >= 7 * table->capacity)
            grow_table(table);
    unsigned int hashval = hash_sv(sv, table);
    HashEntry *entry = get_entry_sv(sv, table, hashval);
    
    if (entry == NULL) {
        entry = malloc(sizeof(HashEntry));
        entry->next = NULL;
        entry->owns_key = deep_copy_key;
        entry->value = malloc(val_size);
        if(deep_copy_key){
            entry->key = sv_from_size(strndup(sv.data,sv.count),sv.count);
        }
        else
            entry->key = sv;
        entry->next = table->entries[hashval];
        table->entries[hashval] = entry;
        table->entry_count++;
        
    } else {
       void *tmp = malloc(val_size);
       if (!tmp)
            return;
        free(entry->value);
        entry->value = tmp;    
    }
    memcpy(entry->value, value, val_size);

}

void *get_from_sv(StringView key, HashTable *table) {
    if (table == NULL || table->entries == NULL)
        return NULL;

    int hashval = hash_sv(key, table);
    HashEntry *entry = get_entry_sv(key, table, hashval);
    if (entry == NULL)
        return NULL;
    return entry->value;
}
void free_table(HashTable **hashtable) {
    if (!hashtable || !(*hashtable))
        return;
    if ((*hashtable)->entries != NULL) {
        for (int i = 0; i < (*hashtable)->capacity; i++) {
            HashEntry *current = (*hashtable)->entries[i];
            while (current != NULL) {
                HashEntry *temp = current;
                current = current->next;
                if(temp->owns_key)
                    free(temp->key.data);
                temp->key.count = 0;
                temp->key.data = NULL;
                free(temp->value);
                free(temp);
            }
        }
    }
    free((*hashtable)->entries);
    free(*hashtable);
    *hashtable = NULL;
}
int get_as_int_sv(StringView key, HashTable *table) {
    void *val = get_from_sv(key, table);
    if (val == NULL)
        return 0;
    return *(int *)val;
}

const char * get_as_cstr_sv(StringView key, HashTable *table) {
    void *val = get_from_sv(key, table);
    if (val == NULL)
        return 0;
    return (const char *)val;
}

float get_as_float_sv(StringView key, HashTable *table) {
    void *val = get_from_sv(key, table);
    if (val == NULL)
        return 0.0f;
    return *(float *)val;
}
StringView get_as_sv(StringView key, HashTable *table) {
    void *val = get_from_sv(key, table);
    if (val == NULL)
        return SV_NULL;
    return *(StringView *)val;
}


