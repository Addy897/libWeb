#include "include/response.h"
#include "include/hash_table.h"
#include "include/helper.h"
#include "include/request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define HEADER_NAME_MAX 128
Response *initResponse() {
  Response *response = malloc(sizeof(Response));
  (response)->status = StatusCodes[0];
  (response)->body = SV_NULL;
  response->owns_body = false;
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

void add_response_header_sv(StringView name, char *value, Response *res) {
  if (res->headers == NULL)
    return;
  
  add_sv(name, value, strlen(value) + 1, res->headers,false);
}
void add_response_header(char *name, char *value, Response *res) {
  if (res->headers == NULL)
    return;
  
  add(name, value, strlen(value) + 1, res->headers);
}
void remove_response_header(char *name, Response *res) {
  if (res->headers == NULL)
    return;

  remove_key(name, res->headers);
}
void remove_response_header_sv(StringView name, Response *res) {
  if (res->headers == NULL)
    return;

  remove_key_sv(name, res->headers);
}
StringView get_response_header_sv(StringView name, Response *res) {
  if (res->headers == NULL)
    return SV_NULL;

  const char * rh = get_as_cstr_sv(name, res->headers);
  if(!rh) return SV_NULL;
  return sv_from_cstr(rh);
}
StringView get_response_header(char *name, Response *res) {
  if (res->headers == NULL)
    return SV_NULL;

  const char * rh = get_as_cstr(name, res->headers);
  if(!rh) return SV_NULL;
  return sv_from_cstr(rh);
}
int get_all_response_headers(char ** headers_str, Response *res) {
  int full_headers_size = 256;
  *headers_str = malloc(sizeof(char) * full_headers_size);
  int it = 0;
  for (int i = 0; i < res->headers->capacity; i++) {
    HashEntry *current = res->headers->entries[i];
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
void set_response_body(char *body, Response *res) { 
    res->owns_body = true;
    res->body = sv_from_cstr(strdup(body));
     
}
void set_response_body_sv(StringView sv, Response *res) { 
    res->owns_body = false;
    res->body = sv; 
}
static int fast_itoa(size_t val, char* buf) {
    char temp[32];
    int pos = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    while (val > 0) {
        temp[pos++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = 0; i < pos; i++) {
        buf[i] = temp[pos - 1 - i];
    }
    buf[pos] = '\0';
    return pos;
}
#define set_response_status(status,status_len,status_buf,res) \
do{ \
    switch(res->status.code){ \
        case 200: \
            status = "HTTP/1.1 200 OK\r\n"; \
            status_len = 17; \
            break; \
        case 404: \
            status = "HTTP/1.1 404 Not Found\r\n"; \
            status_len = 24; \
            break; \
        default: \
            status_len = snprintf(status_buf,sizeof(status_buf),"HTTP/1.1 %d %s\r\n",res->status.code,res->status.message); \
    } \
}while(0)
char *responseToString(int *total_len, Response *res, Method m) {
  const char* status = NULL;
  char status_buf[128];
  int current_len = 0;
  set_response_status(status,current_len,status_buf,res); 
  StringView content_len = get_response_header("content-length", res);
  if (sv_eq(content_len,SV_NULL) && res->body.count >0) {
    char content_len_buf[32];
    fast_itoa(res->body.count,content_len_buf);
    add_response_header("content-length", content_len_buf, res);
  }
  char *full_headers = NULL;
  int headers_len = get_all_response_headers(&full_headers,res);
  if(!full_headers){
       *total_len = 0;
        perror("Headers is NULL");
        return NULL;
  }
  int full_len = current_len + headers_len + res->body.count + 2;
  char *data = malloc(full_len + 1);
  if(!data){
        *total_len = 0;
        perror("calloc for data failed");
        return NULL;

    }
 
  data[full_len] = '\0';
  if(status != NULL)
    memcpy(data, status, current_len);
  else
    memcpy(data, status_buf, current_len);
  memcpy(data + current_len, full_headers, headers_len);
  current_len += headers_len;
  memcpy(data + current_len, "\r\n", 2);
  current_len += 2;
  if (res->body.count> 0 && m != HEAD) {
    memcpy(data + current_len, res->body.data, res->body.count);
    current_len += res->body.count;
  }
  free(full_headers);
  *total_len = current_len;
  return data;
}

void freeResponse(Response **response) {
  if (response == NULL)
    return;
  if ((*response)->owns_body && (*response)->body.data != NULL){
     free((*response)->body.data);
   }
    (*response)->body.count =0;
  if ((*response)->headers != NULL)
    free_table(&(*response)->headers);
  free(*response);
  *response = NULL;
}

void setBodyFromFile(char *pathname, Response *res) {
    StringView body = get_content(pathname,&FILE_CACHE);
    if(!sv_eq(body,SV_NULL)){
        res->body = body;
        return;
    }

     int fd = open(pathname, O_RDONLY);
    if (fd < 0) return;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return;
    }
    
    char * temp = malloc(st.st_size + 1);
    if (temp) {
        read(fd, temp, st.st_size);
        temp[st.st_size] = '\0';
        res->owns_body = false;
        res->body = sv_from_size(temp,st.st_size);
        add_content(pathname,res->body,&FILE_CACHE);
    }
    close(fd);
}


