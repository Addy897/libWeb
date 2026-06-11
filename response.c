#include "include/response.h"
#include "include/hash_table.h"
#include "include/helper.h"
#include "include/request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

Response *initResponse() {
  Response *response = malloc(sizeof(Response));
  (response)->status = StatusCodes[0];
  (response)->body = NULL;
  (response)->headers = init_table(16);
  return response;
}
void setStatus(int status, Response *response) {
  for (int i = 0; i < STATUS_COUNT; i++) {
    if (status == StatusCodes[i].code) {
      response->status = StatusCodes[i];
      break;
    }
  }
}

void add_response_header(char *name, char *value, HashTable *headers) {
  if (headers == NULL)
    return;
  
  add(name, value, strlen(value) + 1, headers);
}
void remove_response_header(char *name, HashTable *headers) {
  if (headers == NULL)
    return;
  remove_key(name, headers);
}
StringView get_response_header(char *name, HashTable *headers) {
  if (headers == NULL)
    return SV_NULL;
  const char * rh = get_as_cstr(name, headers);
  if(!rh) return SV_NULL;
  return sv_from_cstr(rh);
}
int get_all_response_headers(char ** headers_str ,HashTable *headers) {
  int full_headers_size = 256;
  *headers_str = malloc(sizeof(char) * full_headers_size);
  int it = 0;
  for (int i = 0; i < headers->capacity; i++) {
    HashEntry *current = headers->entries[i];
    while (current != NULL) {
      int key_len = current->key.count;
      int val_len = strlen((char*)current->value);
      int final_len = key_len+ val_len + 4; //':' + ' ' + '\r\n'
      if (it + final_len + 1 > full_headers_size) {
        full_headers_size *= 2;
        char *tmp = realloc(*headers_str, full_headers_size);
        if (!tmp) {
            free(*headers_str);
            *headers_str = NULL;
            return 0;
        }
        *headers_str = tmp;
      }
      memcpy(*headers_str + it, current->key.data, key_len);
      it += key_len;
      (*headers_str)[it++] = ':';
      (*headers_str)[it++] = ' ';
      memcpy(*headers_str + it, current->value, val_len);
      it += val_len;
      (*headers_str)[it++] = '\r';
      (*headers_str)[it++] = '\n';

      //sprintf(*headers_str + it, ""SV_Fmt": %s\r\n",SV_Arg(current->key),(char*)current->value);
      //it += final_len;

      current = current->next;
    }
  }
  return it;
}
void setResponseBody(char *body, Response *res) { res->body = strdup(body); }
char *responseToString(int *total_len, Response *res, Method m) {
  char status[128];
  int current_len = snprintf(status,sizeof(status),"HTTP/1.1 %d %s\r\n",res->status.code,res->status.message);

  int body_len = res->body == NULL ? 0 : strlen(res->body);
  StringView content_len = get_response_header("content-length", res->headers);
  if (sv_eq(content_len,SV_NULL) && body_len>0) {
    char content_len_buf[32];
    snprintf(content_len_buf, sizeof(content_len_buf), "%d", body_len);
    add_response_header("content-length", content_len_buf, res->headers);
  }
  char *full_headers = NULL;
  int headers_len = get_all_response_headers(&full_headers,res->headers);
  if(!full_headers){
       *total_len = 0;
        perror("Headers is NULL");
        return NULL;
  }
  int full_len = current_len + headers_len + body_len + 2;
  char *data = malloc(full_len + 1);
  data[full_len] = '\0';
  if(!data){
        *total_len = 0;
        perror("calloc for data failed");
        return NULL;

    }
  memcpy(data, status, current_len);
  memcpy(data + current_len, full_headers, headers_len);
  current_len += headers_len;
  memcpy(data + current_len, "\r\n", 2);
  current_len += 2;
  if (body_len > 0 && m != HEAD) {
    memcpy(data + current_len, res->body, body_len);
    current_len += body_len;
  }
  free(full_headers);
  *total_len = current_len;
  return data;
}

void freeResponse(Response **response) {
  if (response == NULL)
    return;
  if ((*response)->body != NULL)
    free((*response)->body);
  if ((*response)->headers != NULL)
    free_table(&(*response)->headers);
  free(*response);
  *response = NULL;
}

void setBodyFromFile(char *pathname, Response *res) {
    int fd = open(pathname, O_RDONLY);
    if (fd < 0) return;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return;
    }

    res->body = malloc(st.st_size + 1);
    if (res->body) {
        read(fd, res->body, st.st_size);
        res->body[st.st_size] = '\0';
    }
    close(fd);
}


