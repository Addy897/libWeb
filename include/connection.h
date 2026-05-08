#ifndef CONNECTION_H
#define CONNECTION_H

#include "errors.h"
#include "request.h"
#include "response.h"
#include <sys/epoll.h>

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
    char buf[4096];
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

};

union data_u{
    struct req_states req;
    struct res_states res;
};
struct connection_t{
    SOCKET client;
    enum STATE state;
    Request * req;
    Response* res;
    union data_u data;
    struct file_res_states file;
};
typedef struct connection_t Connection;

Connection * init_connection();
int parse_status_line(Request* req,char * line);
int parse_headers(Connection * conn);
int build_request(Connection * conn);
int sendFile(Connection* con,char* filepath);
int sendResponse(Connection* con);

void free_connection(Connection ** con);

#endif
