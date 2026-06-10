#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "include/connection.h"
#include <sys/sendfile.h>
Connection * init_connection(){
    Connection * con = calloc(1,sizeof(Connection));
    return con;
}
void free_connection(Connection ** con){
    if(*con == NULL) return;
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
    if (token == NULL){
        printf("Unable to parse line %s\n",line);
        return ERR_PARSING_FAILED;
    }
    req->method = -1;
    for (int i = 0; i < METHODS_LEN; i++) {
        if (strcmp(token, methods[i]) == 0) {
            req->method = (Method)i;
        }
    }
    if (req->method == -1){
        printf("Unable to parse method\n");
        return ERR_PARSING_FAILED;
    }
    
    char* req_target = strtok_r(saved_line, " ",&saved_line);
    if (req_target == NULL){
        printf("Unable to req_target %s \n",saved_line);
        return ERR_PARSING_FAILED;
    }


    char *version = strtok_r(saved_line, " ",&saved_line);
    if (version == NULL){
        printf("Unable to version %s \n",saved_line);
        return ERR_PARSING_FAILED;
    }
    strncpy(req->version, version,sizeof(req->version)-1);
    req->version[sizeof(req->version) - 1] = '\0';

    char *query_start = strchr(req_target, '?');
    if (query_start != NULL) {
        req->query_params = initTable(16);
        char *path = strtok_r(req_target, "?",&req_target);
        if (!path){
            return ERR_PARSING_FAILED;
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
    return 1;
}

int parse_headers(Connection * conn){
    char * saved_ptr = conn->data.req.buf;
        
    char *line;
    if(conn->req->path == NULL){
        line = strsplit(saved_ptr,"\r\n",&saved_ptr);
        if(line == NULL){
            printf("Line is empty %s \n",saved_ptr);
            return ERR_PARSING_FAILED;
        }
        if(parse_status_line(conn->req,line) <0){
            return ERR_PARSING_FAILED;
        }
    }
    if(conn->req->headers == NULL)
        conn->req->headers = initTable(16);
    while ((line = strsplit(saved_ptr, "\r\n",&saved_ptr)) != NULL) {
        if(*line == '\0'){
            conn->state = PARSING_BODY;
            break;
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
    if (conn == NULL || conn->req == NULL || conn->client == 0){
        return 0;
    }
    if(conn->state == REQUEST_BUILT){
        return 1;
    }

    int bytes_read = 0;
    char buf[1024];
    while((bytes_read = recv(conn->client, buf, sizeof(buf)-1, 0)) > 0){
        buf[bytes_read] = '\0'; 
        if(bytes_read + conn->data.req.pos > sizeof(conn->data.req.buf) - 1){
                return ERR_CONTENT_TOO_LARGE;
        }

        if(conn->state == PARSING_HEADERS){
            if (bytes_read + conn->data.req.pos > sizeof(conn->data.req.buf) - 1) {
                return ERR_CONTENT_TOO_LARGE;
            }
            memcpy(conn->data.req.pos+conn->data.req.buf,buf,bytes_read);
            conn->data.req.pos+=bytes_read;
            conn->data.req.buf[conn->data.req.pos] = '\0';
            int r = parse_headers(conn);
            if(r == ERR_PARSING_FAILED) return r;
        }
        if(conn->state == PARSING_BODY){
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
                      conn->req->body = strdup(buf);
                      conn->req->body_len = bytes_read;
                    } else {
                        char * temp = realloc(conn->req->body, conn->req->body_len + bytes_read+1);
                        if (!temp){
                            print_error("realloc failed!!");
                            free(conn->req->body);
                            conn->req->body = NULL;
                            return ERR_MEMORY_ALLOCATION;
                        }
                        conn->req->body = temp;
                        memcpy(conn->req->body+conn->req->body_len,buf,bytes_read);
                        conn->req->body_len += bytes_read;
                    }
                    if (conn->req->body_len >= content_size) {
                        conn->req->body[conn->req->body_len] = '\0';
                        conn->state = REQUEST_BUILT;
                        return 1;
                    }
              }else{
                    conn->state=REQUEST_BUILT;
                    return 1;
                }
                
            }else{
            
                    conn->state=REQUEST_BUILT;
                    return 1;   
            }
        }
    }
    if(bytes_read<0){
        int e = GET_NET_ERROR;
        if(IS_WOULD_BLOCK(e)){
            return 0;
        }
    }
    return 1;
}
int sendResponse(Connection * con) {
    if(con->data.res.resp_buf == NULL){
        int len = 0;
        char * data;
        data = responseToString(&len, con->res, con->req->method);
        if (!data) return ERR_NO_RESPONSE_DATA;
        con->data.res.total_size = len;
        con->data.res.resp_buf = data;
    }
    
    if(con->data.res.total_size == con->data.res.bytes_sent && con->data.res.total_size > 0){
        free(con->data.res.resp_buf);
        con->data.res.resp_buf = NULL;
        con->state = RESPONSE_SENT;
        return 1;
    }
    char * data = con->data.res.resp_buf + con->data.res.bytes_sent;
    int len = con->data.res.total_size - (int)con->data.res.bytes_sent;
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
        free(con->data.res.resp_buf);
        con->data.res.resp_buf = NULL;
        con->state = RESPONSE_SENT;
        return 1;
    }
  return 0;
}


int sendFile(Connection *con) {
    if(con->state == SENDING_FILE_HEADERS){
        if(con->res == NULL){
            if(con->file.filepath == NULL){
                return ERR_UNKNOWN;
            } 
            char * filepath = con->file.filepath;
            char data[1024];
            char *mime = getMiME(filepath);
            con->res = initResponse();
            addHeader("Content-Type", mime, con->res->headers);
            char s[32];
            int fd = open(filepath, O_RDONLY);
            if (fd == -1) {
                perror("open");
                return -1;
            }
            struct stat st;
            if (fstat(fd, &st) == -1) {
                perror("fstat");
                close(fd);
                return -1;
            }
            con->file.fd = fd;
            con->file.total_size = st.st_size;
            con->file.offset = 0;
            snprintf(s, sizeof(s), "%d", con->file.total_size);
            addHeader("Content-Length", s, con->res->headers);
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
        if(con->file.offset == con->file.total_size){
            close(con->file.fd);
            con->file.fd = -1;
            con->state = RESPONSE_SENT;
            return 1;
        }
        
    }

    return 0;
}
