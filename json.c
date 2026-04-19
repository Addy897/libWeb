#include "include/json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>



typedef struct {
    const char *src;
    int         pos;
    int         error; 
} Parser;

static JsonValue *parse_value(Parser *p); 



static void skip_ws(Parser *p) {
    while (p->src[p->pos] && isspace((unsigned char)p->src[p->pos]))
        p->pos++;
}

static JsonValue *make(JsonType type) {
    JsonValue *v = calloc(1, sizeof(JsonValue));
    if (v) v->type = type;
    return v;
}


static void array_push(JsonValue *arr, JsonValue *item) {
    if (arr->array.count >= arr->array.capacity) {
        arr->array.capacity = arr->array.capacity ? arr->array.capacity * 2 : 4;
        arr->array.items = realloc(arr->array.items,
                                   arr->array.capacity * sizeof(JsonValue *));
        if(arr->array.items == NULL){
            perror("[json] relloc failed");
            return;
        }
    }
    arr->array.items[arr->array.count++] = item;
}






static char *parse_raw_string(Parser *p) {
    if (p->src[p->pos] != '"') { p->error = 1; return NULL; }
    p->pos++; 

    
    int start = p->pos;
    int len   = 0;
    for (int i = start; p->src[i] && p->src[i] != '"'; ) {
        if (p->src[i] == '\\') { i++; if (!p->src[i]) break; }
        i++; len++;
    }

    char *out = malloc(len + 1);
    if (!out) { p->error = 1; return NULL; }
    int n = 0;

    while (p->src[p->pos] && p->src[p->pos] != '"') {
        if (p->src[p->pos] != '\\') {
            out[n++] = p->src[p->pos++];
            continue;
        }
        p->pos++; 
        switch (p->src[p->pos]) {
            case '"':  out[n++] = '"';  break;
            case '\\': out[n++] = '\\'; break;
            case '/':  out[n++] = '/';  break;
            case 'n':  out[n++] = '\n'; break;
            case 'r':  out[n++] = '\r'; break;
            case 't':  out[n++] = '\t'; break;
            case 'b':  out[n++] = '\b'; break;
            case 'f':  out[n++] = '\f'; break;
            case 'u':
                
                out[n++] = '?';
                for (int k = 0; k < 4 && p->src[p->pos + 1]; k++) p->pos++;
                break;
            default:
                out[n++] = p->src[p->pos];
                break;
        }
        p->pos++;
    }
    out[n] = '\0';

    if (p->src[p->pos] == '"') p->pos++; 
    else { free(out); p->error = 1; return NULL; }

    return out;
}



static JsonValue *parse_string(Parser *p) {
    char *s = parse_raw_string(p);
    if (!s) return NULL;
    JsonValue *v = make(JSON_STRING);
    v->string = s;
    return v;
}

static JsonValue *parse_number(Parser *p) {
    const char *start = p->src + p->pos;

    
    int is_float = 0;
    int i = p->pos;
    if (p->src[i] == '-') i++;
    while (isdigit((unsigned char)p->src[i])) i++;
    if (p->src[i] == '.' || p->src[i] == 'e' || p->src[i] == 'E')
        is_float = 1;

    char *end;
    JsonValue *v;
    if (is_float) {
        v = make(JSON_FLOAT);
        v->number = strtof(start, &end);
    } else {
        v = make(JSON_INT);
        v->integer = (int)strtol(start, &end, 10);
    }
    p->pos += (int)(end - start);
    return v;
}

static JsonValue *parse_object(Parser *p) {
    p->pos++; 
    JsonValue *v = make(JSON_OBJECT);
    v->object = initTable(16);

    skip_ws(p);
    if (p->src[p->pos] == '}') { p->pos++; return v; }

    while (p->src[p->pos] && !p->error) {
        skip_ws(p);
        
        char *key = parse_raw_string(p);
        if (!key) break;

        skip_ws(p);
        if (p->src[p->pos] != ':') { free(key); p->error = 1; break; }
        p->pos++; 

        skip_ws(p);
        JsonValue *val = parse_value(p);
        if (val) {
            
            add(key, &val, sizeof(JsonValue *), v->object);
        }
        free(key); 

        skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == '}') { p->pos++; break; }
        p->error = 1; break;
    }
    return v;
}

static JsonValue *parse_array(Parser *p) {
    p->pos++; 
    JsonValue *v = make(JSON_ARRAY);

    skip_ws(p);
    if (p->src[p->pos] == ']') { p->pos++; return v; }

    while (p->src[p->pos] && !p->error) {
        skip_ws(p);
        JsonValue *item = parse_value(p);
        if (item) array_push(v, item);

        skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == ']') { p->pos++; break; }
        p->error = 1; break;
    }
    return v;
}

static JsonValue *parse_value(Parser *p) {
    skip_ws(p);
    char c = p->src[p->pos];
    if (p->error) return NULL;

    if (c == '"') return parse_string(p);
    if (c == '{') return parse_object(p);
    if (c == '[') return parse_array(p);
    if (c == '-' || isdigit((unsigned char)c)) return parse_number(p);

    if (strncmp(p->src + p->pos, "true",  4) == 0) {
        p->pos += 4;
        JsonValue *v = make(JSON_BOOL); v->boolean = 1; return v;
    }
    if (strncmp(p->src + p->pos, "false", 5) == 0) {
        p->pos += 5;
        JsonValue *v = make(JSON_BOOL); v->boolean = 0; return v;
    }
    if (strncmp(p->src + p->pos, "null",  4) == 0) {
        p->pos += 4;
        return make(JSON_NULL);
    }

    p->error = 1;
    return NULL;
}



JsonValue *json_parse(const char *src) {
    if (!src) return NULL;
    Parser p = { src, 0, 0 };
    JsonValue *root = parse_value(&p);
    if (p.error) { json_free(root); return NULL; }
    return root;
}

JsonValue *json_get(JsonValue *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    
    void *slot = get((char *)key, obj->object);
    if (!slot) return NULL;
    return *(JsonValue **)slot;
}

JsonValue *json_index(JsonValue *arr, int i) {
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (i < 0 || i >= arr->array.count) return NULL;
    return arr->array.items[i];
}

void json_free(JsonValue *val) {
    if (!val) return;
    switch (val->type) {
        case JSON_STRING:
            free(val->string);
            break;
        case JSON_ARRAY:
            for (int i = 0; i < val->array.count; i++)
                json_free(val->array.items[i]);
            free(val->array.items);
            break;
        case JSON_OBJECT:
            
            
            for (int i = 0; i < val->object->capacity; i++) {
                HashEntry *e = val->object->entries[i];
                while (e) {
                    json_free(*(JsonValue **)e->value);
                    e = e->next;
                }
            }
            freeTable(&val->object);
            break;
        default:
            break;
    }
    free(val);
}

void json_print(JsonValue *val, int indent) {
    if (!val) { printf("(null)"); return; }
    switch (val->type) {
        case JSON_NULL:   printf("null");  break;
        case JSON_BOOL:   printf("%s", val->boolean ? "true" : "false"); break;
        case JSON_INT:    printf("%d",  val->integer); break;
        case JSON_FLOAT:  printf("%g",  val->number);  break;
        case JSON_STRING: printf("\"%s\"", val->string); break;
        case JSON_ARRAY:
            printf("[\n");
            for (int i = 0; i < val->array.count; i++) {
                printf("%*s", (indent + 1) * 2, "");
                json_print(val->array.items[i], indent + 1);
                printf(i < val->array.count - 1 ? ",\n" : "\n");
            }
            printf("%*s]", indent * 2, "");
            break;
        case JSON_OBJECT:
            printf("{\n");
            for (int i = 0; i < val->object->capacity; i++) {
                for (HashEntry *e = val->object->entries[i]; e; e = e->next) {
                    printf("%*s\"%s\": ", (indent + 1) * 2, "", e->key);
                    json_print(*(JsonValue **)e->value, indent + 1);
                    printf("\n"); 
                }
            }
            printf("%*s}", indent * 2, "");
            break;
    }
}
