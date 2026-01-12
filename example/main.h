typedef enum { GET = 0, POST = 1 } Method;
struct HashTable;

typedef struct {
  Method method;
  char version[32];
  int body_len;
  char *path;
  char *body;
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
void addHeader(char *name, char *value, struct HashTable *headers);
void removeHeader(char *name, struct HashTable *headers);
const char *getHeader(char *name, struct HashTable *headers);
const char *getParams(char *name, struct HashTable *params);
char *getAllHeaders(struct HashTable *headers);
void setResponseBody(char *body, Response *);
void cleanupRoutes();
