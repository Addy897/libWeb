#include "include/connection.h"
#include <sys/sendfile.h>
Connection * init_connection(){
    Connection * con = calloc(1,sizeof(Connection));
    return con;
}
void free_connection(Connection ** con){
    Connection * conn = *con;
    closesocket(conn->client);
    if(conn->req){
        freeRequest(&conn->req);
    }
    if(conn->res){
        freeResponse(&conn->res);
    }
    free(conn);
    *con = NULL;
}


int parse_status_line(Request* req,char * line){
    char * token ,*saved_line;
    token = strtok_r(line, " ",&saved_line);
    if (token == NULL)
        return 0;
    req->method = -1;
    for (int i = 0; i < METHODS_LEN; i++) {
        if (strcmp(token, methods[i]) == 0) {
            req->method = (Method)i;
        }
    }
    if (req->method == -1)
        return 0;
    
    char* req_target = strtok_r(saved_line, " ",&saved_line);
    if (req_target == NULL)
        return 0;


    char *version = strtok_r(saved_line, " ",&saved_line);
    if (version == NULL)
        return 0;
    strncpy(req->version, version,sizeof(req->version)-1);
    req->version[sizeof(req->version) - 1] = '\0';

    char *query_start = strchr(token, '?');
    if (query_start != NULL) {
        req->query_params = initTable(16);
        char *path = strtok_r(req_target, "?",&req_target);
        if (!path){
            return 0;
        }
        req->path = strdup(path);
        char * query;
        while ((query = strtok_r(req_target, "&", &req_target)) != NULL) {
            char *key = strtok_r(query, "=",&query);
            if (!key || !query || *query == '\0')
                continue;
            key = trim(key);
            query= trim(query);
            add(key, query, strlen(query)+1, req->query_params);
        }
    } else {
        req->path = strdup(req_target);
    }
}
int parse_headers(Connection * conn){
    char * saved_ptr = conn->data.req.buf;
        
    char *line;
    if(conn->req->path == NULL){
        line = strsplit(saved_ptr,"\r\n",&saved_ptr);
        if(line == NULL) return 0;
        if(parse_status_line(conn->req,line) <=0){
            return ERR_PARSING_FAILED;
        }
    }
    if(conn->req->headers == NULL)
        conn->req->headers = initTable(16);
    while ((line = strsplit(saved_ptr, "\r\n",&saved_ptr)) != NULL) {
        if(*line == '\0'){
            conn->state = PARSING_BODY;
        }
        char *key = strsplit(line, ":",&line);
        if (!key || !line || *line =='\0')
            continue;
        key = trim(key);
        line=trim(line);
        toLowerCase(key);
        add(key, line, strlen(line)+1, conn->req->headers);
    }
    int left_size = strlen(saved_ptr);
    if(left_size > 0){
        memmove(conn->data.req.buf,saved_ptr,left_size);
        conn->data.req.pos = left_size;
        conn->data.req.buf[left_size] = '\0';
    }else{
        conn->data.req.pos = 0;
        conn->data.req.buf[0] = '\0';
    }
    if (conn->state == PARSING_BODY && conn->data.req.pos > 0) {
        conn->req->body_len = conn->data.req.pos;
        conn->req->body = calloc(1, conn->req->body_len + 1);
        if(!conn->req->body) return ERR_MEMORY_ALLOCATION;
        
        memcpy(conn->req->body, conn->data.req.buf, conn->req->body_len);
        conn->data.req.pos = 0;     
    }
     
    return 0;

}

int build_request(Connection * conn) {
    if (conn == NULL || conn->req == NULL || conn->client == 0)
        return 0;
    if(conn->state == REQUEST_BUILT){
        return 0;
    }

    int bytes_read = 0;
    char buf[1024];
    while((bytes_read = recv(conn->client, buf, sizeof(buf)-1, 0)) > 0){
        buf[bytes_read] = '\0'; 
        if(bytes_read + conn->data.req.pos > sizeof(buf) - 1){
                return ERR_CONTENT_TOO_LARGE;
        }

        memcpy(conn->data.req.pos+conn->data.req.buf,buf,bytes_read);
        conn->data.req.pos+=bytes_read;
        if(conn->state == PARSING_HEADERS){
            parse_headers(conn);
        }else if(conn->state == PARSING_BODY){
            //RAW COPY FOR NOW
            const char *content_string = getAsString("content-length", conn->req->headers);
            if (content_string) {
                long content_size = strtol(content_string,NULL,10);
                if(content_size<0 || content_size >=MAX_CONTENT_SIZE){
                    conn->state = SENDING_RESPONSE;
                    conn->res = initResponse();
                    setStatus(413, conn->res);
                    setResponseBody("Content Too Large", conn->res);
                    return ERR_CONTENT_TOO_LARGE; 
                }
               if(conn->req->body_len < content_size){ 
                   if (!conn->req->body) {
                      conn->req->body = strdup(conn->data.req.buf);
                      conn->req->body_len = 0;
                    } else {
                      conn->req->body = realloc(conn->req->body, conn->req->body_len + conn->data.req.pos);
                      if (!conn->req->body){
                        perror("realloc failed!!");
                        return ERR_MEMORY_ALLOCATION;
                        }
                    }
                    conn->req->body_len += conn->data.req.pos;
              }else{
                    conn->state=REQUEST_BUILT;
                }
                
          }else{
            
                    conn->state=REQUEST_BUILT;
          }
           

        }
         
    }
    if(bytes_read == 0){
        return ERR_UNKNOWN;
    }else{
        int e = GET_NET_ERROR;
        if(IS_WOULD_BLOCK(e)){
            return 0;
        }
    }
    return 1;
}
int sendResponse(Connection * con) {
  int len = 0;
  char * data;

  if(con->data.res.resp_buf == NULL){
      data = responseToString(&len, con->res, con->req->method);
      if (!data)
        return ERR_NO_RESPONSE_DATA;
    con->data.res.total_size = len;
    con->data.res.resp_buf = data;
  }
 if(con->data.res.total_size == con->data.res.bytes_sent && con->data.res.total_size > 0){
    free(data);
    con->state = RESPONSE_SENT;
    return 1;
 }

  data = con->data.res.resp_buf + con->data.res.bytes_sent;
  int ret = send(con->client, data, len, 0);
  if(ret > 0){
    con->data.res.bytes_sent+=ret;
  }
  if (ret < 0) {
    int e = GET_NET_ERROR;
    if(IS_WOULD_BLOCK(e)){
        return 0;
    }
    printf("[sendResponse] ret = %d\n", ret);
  }
 if(con->data.res.total_size == con->data.res.bytes_sent){
    free(data);
    con->state = RESPONSE_SENT;
    return 1;
  }
  return 0;
}


int sendFile(Connection *con,char* filepath) {
    if(con->state == SENDING_FILE_HEADERS){
        if(con->res == NULL){ 
            char data[1024];
            FILE *fptr = fopen(filepath, "rb");
            if (!fptr) {
              printf("Error no such file: %s\n", filepath);
              return -1;
            }
            fseek(fptr, 0L, SEEK_END);
            long size = ftell(fptr);
            rewind(fptr);
            char *mime = getMiME(filepath);
            con->res = initResponse();
            addHeader("Content-Type", mime, con->res->headers);
            char s[32];
            snprintf(s, sizeof(s), "%d", size);
            addHeader("Content-Length", s, con->res->headers);
            con->file.fd = fileno(fptr);
            con->file.offset = 0;
            con->file.total_size =size;
        }
        sendResponse(con);
        if(con->state == RESPONSE_SENT){
            con->state = SENDING_FILE;
        }
    }
    if(con->state == SENDING_FILE){
        int bytes_sent = sendfile(con->client,con->file.fd,&con->file.offset,con->file.total_size);
        if(bytes_sent<0){
            int e = GET_NET_ERROR;
            if(IS_WOULD_BLOCK(e)){
                return 0;
            }
            print_error("Send file");
            return -1;
        }
        con->file.offset +=bytes_sent;
        if(con->file.offset == con->file.total_size){
            con->state = RESPONSE_SENT;
            return 1;
        }
        
    }

    return 0;
}
