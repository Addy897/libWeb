struct HashTable;
extern int initializeSocket();
extern void setStaticPath(char *path);
typedef struct Request {
  char method[10];
  char path[100];
  char version[20];
  char headers[20][100];
  int header_count;
  char body[1024];
  char query_params[20][100];
  int query_param_count;
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
extern void setBodyFromFile(char *pathname, Response *res);
extern void addRoute(char *path, void (*callbackfunc)(Request *, Response *));
extern void startServer(char *addr, int port);
extern void setStatus(int status, Response *response);
extern void addHeader(char *name, char *value, Response *response);
extern void removeHeader(char *name, Response *response);
extern const char *getHeader(char *name, Response *response);
extern char *getAllHeaders(Response *response);
extern void setBody(char *body, Response *);
extern void cleanupRoutes();
