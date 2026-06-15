#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "connection.h"
#include "compat.h"
#include "globals.h"
#include "string_view.h"
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


int parse_status_line(Request* req,StringView line){
    StringView token,saved_line;
    token = sv_trim(sv_split(&line,' '));
    if (sv_eq(token,SV_NULL)){
        printf("Unable to parse line "SV_Fmt" \n",SV_Arg(token));
        return ERR_PARSING_FAILED;
    }
    req->method = -1;
    for (int i = 0; i < METHODS_LEN; i++) {
        if (sv_eq(token, sv_from_cstr(methods[i]))) {
            req->method = (Method)i;
        }
    }
    if (req->method == -1){
        printf("Unable to parse method\n");
        return ERR_PARSING_FAILED;
    }
    
    StringView req_target = sv_trim(sv_split(&line,' '));
    if (sv_eq(req_target,SV_NULL)){
        printf("Unable to parse path "SV_Fmt" \n",SV_Arg(req_target));
        return ERR_PARSING_FAILED;
    }


    StringView version = sv_trim(sv_split(&line,' '));
    if (sv_eq(version,SV_NULL)){
        printf("Unable to version "SV_Fmt" \n",SV_Arg(version));
        return ERR_PARSING_FAILED;
    }
    req->version = version;
    token = sv_trim(sv_split(&req_target,'?'));
    req->path = token;
    HashTable * queries;
    if(!sv_eq(req_target,SV_NULL)){
        queries = init_table(16);
        req->query_params = queries;
    }
    while (!sv_eq(req_target,SV_NULL)) {
        StringView query = sv_split(&req_target,'&');
        StringView key = sv_split(&query, '=');
        key = sv_trim(key);
        query= sv_trim(query);
        if (sv_eq(key,SV_NULL) || sv_eq(query,SV_NULL))
            continue;
        add_sv(key,&query,sizeof(StringView),queries,false);
    }
    return 1;
}

int parse_headers(Connection * conn){
    StringView saved_ptr = sv_from_size(conn->data.req.buf,conn->data.req.pos);
    if(sv_eq(conn->req->path,SV_NULL)){
        StringView line = sv_split(&saved_ptr,'\n');
        if(sv_eq(saved_ptr,SV_NULL)){
            printf("Line is empty %s \n",saved_ptr);
            return ERR_PARSING_FAILED;
        }
        if(parse_status_line(conn->req,line) <0){
            return ERR_PARSING_FAILED;
        }
    }
    if(conn->req->headers == NULL){
        conn->req->headers = init_table(16);
    }
    while (!sv_eq(saved_ptr,SV_NULL)) {
        StringView line = sv_split(&saved_ptr,'\n');
        if(sv_is_empty(line)){
            conn->state = PARSING_BODY;
            break;
        }
        StringView key = sv_trim(sv_split(&line,':'));
        line = sv_trim(line);
        if(sv_eq(key,SV_NULL) || sv_eq(line,SV_NULL))
            continue;
       
        add_sv(key, &line, sizeof(StringView), conn->req->headers,false);
    }
    conn->state = PARSING_BODY;
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
    int total_size = sizeof(conn->data.req.buf);
    int read_size = 4096;
    while(1){
        int space_left = BUFFER_MAX - conn->data.req.pos - 1;

        if (space_left <= 0) {
            return ERR_CONTENT_TOO_LARGE;
        }

        bytes_read = recv(conn->client, conn->data.req.buf + conn->data.req.pos, space_left, 0); 
        
        if (bytes_read <= 0) {
            break;
        }
        
        conn->data.req.pos += bytes_read;
        if(conn->state == PARSING_HEADERS){
            int r = parse_headers(conn);
            if(r == ERR_PARSING_FAILED) return r;
        }
        if(conn->state == PARSING_BODY){
            //RAW COPY FOR NOW
            StringView content_string = get_as_sv_s("content-length", conn->req->headers);
            if (!sv_eq(content_string,SV_NULL)) {
                int content_size = sv_to_int(content_string);
                if(content_size<0 || content_size >=MAX_CONTENT_SIZE){
                    conn->state = SENDING_RESPONSE;
                    conn->res = initResponse();
                    setStatus(413, conn->res);
                    set_response_body_sv(sv_from_cltr("Content Too Large"), conn->res);
                    return ERR_CONTENT_TOO_LARGE; 
                }
               if(conn->req->body.count < content_size){ 
                   if (sv_eq(conn->req->body,SV_NULL)) {
                      conn->req->body = sv_from_size(conn->data.req.buf,conn->data.req.pos);
                      StringView temp = sv_split_sv(&conn->req->body,sv_from_cltr("\r\n\r\n")); 
                    } else {
                        if(conn->data.req.buf + conn->data.req.pos > conn->req->body.data + conn->req->body.count) 
                            conn->req->body.count += bytes_read;
                    }
                    if (conn->req->body.count >= content_size) {
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
int sendResponse(Connection * con,HashTable * cache) {
    
    if(con->data.res.resp_buf == NULL){
        if(cache !=NULL && con->req->method == GET){
            StringView sv = get_as_sv(con->req->path,cache);
            if(!sv_eq(sv,SV_NULL)){
               con->data.res.resp_buf = sv.data;
               con->data.res.total_size = sv.count;
            }else{
                goto create;
            }
        }else{
        create:
            int len = 0;
            char * data;
            data = responseToString(&len, con->res, con->req->method);
            if (!data) return ERR_NO_RESPONSE_DATA;
            con->data.res.total_size = len;
            con->data.res.resp_buf = data;
            StringView sv = sv_from_size(data,len);
            add_sv(con->req->path,&sv,sizeof(StringView),cache,true);
        }
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
        con->data.res.resp_buf = NULL;
        con->data.res.total_size = 0;
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
            add_response_header("content-type", mime, con->res);
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
            add_response_header("content-length", s, con->res);
        }
        sendResponse(con,NULL);
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
