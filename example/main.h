struct HashTable;
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

int initializeSocket();
void setPublicDir(char *path);
void setBodyFromFile(char *pathname, Response *res);
void addRoute(char *path, void (*callbackfunc)(Request *, Response *));
void startServer(char *addr, int port);
void setStatus(int status, Response *response);
void addHeader(char *name, char *value, Response *response);
void removeHeader(char *name, Response *response);
const char *getHeader(char *name, Response *response);
char *getAllHeaders(Response *response);
void setBody(char *body, Response *);
void cleanupRoutes();
