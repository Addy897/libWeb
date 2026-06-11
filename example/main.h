#include "string_view.h"

#define getParams(name, params) getHeader(name, params)
typedef enum { GET = 0, POST = 1, HEAD = 2 } Method;
struct HashTable;

typedef struct {
  Method method;
  StringView version;
  StringView path;
  StringView body;
  struct HashTable *headers;
  struct HashTable *query_params;
} Request;


typedef struct {
  int code;
  const char *message;
} StatusCode;
typedef struct {
  StatusCode status;
  struct HashTable *headers;
  char *body;
} Response;

int initializeSocket();
void setPublicDir(char *path);
void setBodyFromFile(char *pathname, Response *res);
void addRoute(Method m, char *path,
              void (*callbackfunc)(Request *, Response *));
void startServer(char *addr, int port);
void setStatus(int status, Response *response);
void add_response_header(char *name, char *value, struct HashTable *headers);
void remove_response_header(char *name, struct HashTable *headers);
StringView get_response_header(char *name, struct HashTable *headers);
StringView get_request_header(char *name, struct HashTable *headers);
int get_all_response_headers(char ** ,struct HashTable *headers);
void setResponseBody(char *body, Response *);
void cleanupRoutes();
