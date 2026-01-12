#include "include/response.h"
#include "include/hash_table.h"
#include "include/helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Response *initResponse() {
  Response *response = malloc(sizeof(Response));
  (response)->status = StatusCodes[0];
  (response)->body = nullptr;
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
      HashEntry *temp = current;
      current = current->next;
      int key_len = strlen(temp->key);
      int val_len = strlen(temp->value);
      int final_len = key_len + val_len + 3; //':' + ' ' + '\n' + '\0?'
      if (it + final_len >= full_headers_size) {
        full_headers_size *= 2;
        headers_str = realloc(headers_str, sizeof(char) * full_headers_size);
      }
      sprintf(headers_str + it, "%s: %s\n", temp->key, (char *)temp->value);
      it += final_len;
    }
  }
  return headers_str;
}
void setResponseBody(char *body, Response *res) { res->body = strdup(body); }
char *responseToString(Response *res) {
  int max_len = 1024;
  char *data = malloc(max_len);
  sprintf(data, "HTTP/1.0 %d %s\n", res->status.code, res->status.message);
  int current_len = strlen(data);
  char *full_headers = getAllHeaders(res->headers);
  int headers_len = strlen(full_headers);
  int body_len = res->body == NULL ? 0 : strlen(res->body);
  if (current_len + headers_len + body_len >= max_len) {
    max_len = max_len + (current_len + headers_len + body_len - max_len);
    data = realloc(data, max_len);
  }
  if (res->body != NULL)
    sprintf(data + current_len, "%s\n%s", full_headers, res->body);
  else
    sprintf(data + current_len, "%s\n", full_headers);
  free(full_headers);
  return data;
}
int sendResponse(SOCKET *c, Response *res) {
  char *data = responseToString(res);
  return send(*c, data, strlen(data), 0);
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
  FILE *file = fopen(pathname, "r");
  if (file != NULL) {

    res->body = (char *)malloc(1);
    if (res->body == NULL) {
      fclose(file);
      return;
    }
    res->body[0] = '\0';

    char buffer[1024];
    size_t totalBytesRead = 0;
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      char *temp = realloc(res->body, totalBytesRead + bytesRead + 1);
      if (temp == NULL) {
        free(res->body);
        fclose(file);
        return;
      }
      res->body = temp;
      memcpy(res->body + totalBytesRead, buffer, bytesRead);
      totalBytesRead += bytesRead;
    }
    res->body[totalBytesRead] = '\0';
    fclose(file);
  }
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
  itoa(size, s, 10);
  addHeader("Content-Length", s, res->headers);
  char *d = responseToString(res);
  send(client, d, strlen(d), 0);
  free(d);
  freeResponse(&res);
  size_t bytes_read;
  size_t total_sent = 0;
  char buffer[1024];
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
