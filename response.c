#include "include/response.h"
#include "include/hash_table.h"
#include "include/helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Response *initResponse() {
  Response *response = malloc(sizeof(Response));
  (response)->status = StatusCodes[0];
  (response)->body = NULL;
  (response)->headers = initTable(16);
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
  removeKey(name, headers);
}
const char *getHeader(char *name, HashTable *headers) {
  if (headers == NULL)
    return NULL;
  return getAsString(name, headers);
}
char *getAllHeaders(HashTable *headers) {
  int full_headers_size = 256;
  char *headers_str = malloc(sizeof(char) * full_headers_size);
  int it = 0;
  for (int i = 0; i < headers->capacity; i++) {
    HashEntry *current = headers->entries[i];
    while (current != NULL) {
      int key_len = strlen(current->key);
      int val_len = strlen((char *)current->value);
      int final_len = key_len + val_len + 4; //':' + ' ' + '\r\n'
      if (it + final_len + 1 > full_headers_size) {
        full_headers_size *= 2;
        headers_str = realloc(headers_str, sizeof(char) * full_headers_size);
        if (!headers_str) {
          free(headers_str);
          return NULL;
        }
      }
      sprintf(headers_str + it, "%s: %s\r\n", current->key,
              (char *)current->value);
      it += final_len;

      current = current->next;
    }
  }
  return headers_str;
}
void setResponseBody(char *body, Response *res) { res->body = strdup(body); }
char *responseToString(int *total_len, Response *res) {
  char status[128];
  sprintf(status, "HTTP/1.1 %d %s\r\n", res->status.code, res->status.message);
  int current_len = strlen(status);

  int body_len = res->body == NULL ? 0 : strlen(res->body);
  const char *content_len = getHeader("Content-Length", res->headers);
  if (!content_len) {
    char content_len_buf[32];
    snprintf(content_len_buf, sizeof(content_len_buf), "%d", body_len);

    addHeader("Content-Length", content_len_buf, res->headers);
  }
  char *full_headers = getAllHeaders(res->headers);
  int headers_len = strlen(full_headers);

  int full_len = current_len + headers_len + body_len + 2;
  char *data = calloc(full_len + 1, sizeof(char));
  memcpy(data, status, current_len);
  memcpy(data + current_len, full_headers, headers_len);
  current_len += headers_len;
  memcpy(data + current_len, "\r\n", 2);
  current_len += 2;
  if (body_len > 0) {
    memcpy(data + current_len, res->body, body_len);
    current_len += body_len;
  }
  free(full_headers);
  *total_len = current_len;
  return data;
}
int sendResponse(SOCKET *c, Response *res) {
  int len = 0;
  char *data = responseToString(&len, res);
  int ret = send(*c, data, len, 0);
  if (ret < 0) {
    printf("[sendResponse] ret = %d\n", ret);
  }
  free(data);
  return ret;
}
void freeResponse(Response **response) {
  if (response == NULL)
    return;
  if ((*response)->body != nullptr)
    free((*response)->body);
  if ((*response)->headers != nullptr)
    freeTable(&(*response)->headers);
  free(*response);
  *response = NULL;
}
void setBodyFromFile(char *pathname, Response *res) {

  FILE *file = fopen(pathname, "rb");
  if (file == NULL)
    return;

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  res->body = malloc(size + 1);
  if (res->body) {
    fread(res->body, 1, size, file);
    res->body[size] = '\0';
  }
  fclose(file);
}
int sendFile(SOCKET client, char *filepath) {
  char WEB_PATH[256];
  getPublicDir(WEB_PATH);
  strncat(WEB_PATH, filepath, strlen(filepath));

  char data[1024];
  FILE *fptr = fopen(WEB_PATH, "rb");
  if (!fptr) {
    printf("Error no such file: %s\n", WEB_PATH);
    return 0;
  }
  fseek(fptr, 0L, SEEK_END);
  int size = ftell(fptr);
  rewind(fptr);
  char *mime = getMiME(filepath);
  Response *res = initResponse();
  addHeader("Content-Type", mime, res->headers);
  char s[32];
  snprintf(s, sizeof(s), "%d", size);
  addHeader("Content-Length", s, res->headers);
  int len = 0;
  char *d = responseToString(&len, res);
  send(client, d, len, 0);
  free(d);
  freeResponse(&res);
  size_t bytes_read;
  size_t total_sent = 0;
  char buffer[65536];
  while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), fptr)) > 0) {
    size_t bytes_sent = send(client, buffer, bytes_read, 0);
    if (bytes_sent == -1) {
      perror("Error sending data");
      return 0;
    }
    total_sent += bytes_sent;
    if (total_sent >= size) {
      break;
    }
  }
  fclose(fptr);
  return 1;
}
