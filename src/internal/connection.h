#ifndef CONNECTION_H
#define CONNECTION_H

#include "errors.h"
#include "request.h"
#include "response.h"
#include "globals.h"
#include "mime_types.h"
#include "helper.h"

#include <sys/epoll.h>

#define BUFFER_MAX 8192


enum STATE {
    PARSING_HEADERS,
    PARSING_BODY,
    REQUEST_BUILT,
    SENDING_RESPONSE,
    SENDING_FILE_HEADERS,
    SENDING_FILE,
    RESPONSE_SENT
};

struct req_states{
    char buf[BUFFER_MAX];
    int pos;
};
struct res_states{
    char *resp_buf; 
    int total_size;
    size_t bytes_sent;
};
struct file_res_states{
    int fd;
    off_t offset;
    size_t total_size;
    char filepath[PATH_MAX];

};

typedef struct data_u{
    struct req_states req;
    struct res_states res;
}data_u;
struct connection_t{
    SOCKET client;
    int index;
    bool is_free;
    enum STATE state;
    Request * req;
    Response* res;
    data_u data;
    
    struct file_res_states file;
    
};
typedef struct connection_t Connection;

Connection * init_connection();
int parse_status_line(Request* req,StringView line);
int parse_headers(Connection * conn);
int build_request(Connection * conn);
int sendFile(Connection* con);
int sendResponse(Connection* con,HashTable * cache);

void free_connection(Connection ** con);

#endif
