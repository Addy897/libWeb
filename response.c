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

void addHeader(char *name, char *value, HashTable *headers) {
  if (headers == NULL)
    return;
  add(name, value, strlen(value) + 1, headers);
}
void removeHeader(char *name, HashTable *headers) {
  if (headers == NULL)
    return;
  remove_key(name, headers);
}
StringView getHeader(char *name, HashTable *headers) {
  if (headers == NULL)
    return SV_NULL;
  return get_as_sv_s(name, headers);
}
char *getAllHeaders(HashTable *headers) {
  int full_headers_size = 256;
  char *headers_str = malloc(sizeof(char) * full_headers_size);
  int it = 0;
  for (int i = 0; i < headers->capacity; i++) {
    HashEntry *current = headers->entries[i];
    while (current != NULL) {
      int key_len = current->key.count;
      int val_len = strlen((char*)current->value);
      int final_len = key_len+ val_len + 4; //':' + ' ' + '\r\n'
      if (it + final_len + 1 > full_headers_size) {
        full_headers_size *= 2;
        headers_str = realloc(headers_str, sizeof(char) * full_headers_size);
        if (!headers_str) {
          free(headers_str);
          return NULL;
        }
      }
      sprintf(headers_str + it, ""SV_Fmt": %s\r\n",SV_Arg(current->key),(char*)current->value);
      it += final_len;

      current = current->next;
    }
  }
  return headers_str;
}
void setResponseBody(char *body, Response *res) { res->body = strdup(body); }
char *responseToString(int *total_len, Response *res, Method m) {
  char status[128];
  sprintf(status, "HTTP/1.1 %d %s\r\n", res->status.code, res->status.message);
  int current_len = strlen(status);

  int body_len = res->body == NULL ? 0 : strlen(res->body);
  StringView  content_len = getHeader("content-length", res->headers);
  if (sv_eq(content_len,SV_NULL) && body_len > 0) {
    char content_len_buf[32];
    snprintf(content_len_buf, sizeof(content_len_buf), "%d", body_len);
    addHeader("Content-Length", content_len_buf, res->headers);
  }
  char *full_headers = getAllHeaders(res->headers);
  if(!full_headers){
       *total_len = 0;
        perror("Headers is NULL");
        return NULL;
    }
        int headers_len = strlen(full_headers);

  int full_len = current_len + headers_len + body_len + 2;
  char *data = calloc(full_len + 1, sizeof(char));
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

