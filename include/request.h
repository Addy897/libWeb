#ifndef REQUEST_H
#define REQUEST_H
struct Request {
    char method[10];
    char path[100];
    char version[20];
    char headers[20][100];
    int header_count;
    char body[1024];
    char query_params[20][100];
    int query_param_count;
};
typedef struct Request Request;
#endif