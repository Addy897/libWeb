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
  int status;
  char *headers;
  char *body;
} Response;

extern void addRoute(char *path, char *(*callbackfunc)(Request *, Request *));
extern void startServer(char *addr, int port);
extern char *render_template(char *pathname);

extern void cleanupRoutes();
