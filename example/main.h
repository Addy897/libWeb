typedef enum { GET, POST } Methods;

struct HashTable;
typedef struct {
  Methods method;
  char *path;
  char version[20];
  char *body;
  int body_len;
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
void addRoute(Methods, char *path, void (*callbackfunc)(Request *, Response *));
void startServer(char *addr, int port);
void setStatus(int status, Response *response);
void addHeader(char *name, char *value, struct HashTable *headers);
void removeHeader(char *name, struct HashTable *headers);
const char *getHeader(char *name, struct HashTable *headers);
const char *getParams(char *name, struct HashTable *params);
char *getAllHeaders(struct HashTable *headers);
void setResponseBody(char *body, Response *);
void cleanupRoutes();
