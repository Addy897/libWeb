#pragma once
#include "hash_table.h"

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_INT,
    JSON_FLOAT,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} JsonType;

typedef struct JsonValue {
    JsonType type;
    union {
        int        boolean;   
        int        integer;   
        float      number;    
        char      *string;    
        HashTable *object;    
        struct {
            struct JsonValue **items;
            int count;
            int capacity;
        } array;              
    };
} JsonValue;


JsonValue *json_parse(const char *src);
JsonValue *json_get(JsonValue *obj, const char *key);
JsonValue *json_index(JsonValue *arr, int i);

void json_free(JsonValue *val);

void json_print(JsonValue *val, int indent);
