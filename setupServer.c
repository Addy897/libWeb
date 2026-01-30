#include "include/setupServer.h"
#include "include/request.h"
#include "include/response.h"
#include "include/routing.h"
#ifdef _WIN32
#include <winerror.h>
#endif

void handleRequest(SOCKET c, Request *req) {
  char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found</title>"
                     "</head>"
                     "<body>404 NOT FOUND!!</body>"
                     "</html>";
  const char *method = methods[req->method];
  if (exists(req->path)) {

    int result = sendFile(c, req->path, req->method);

    if (!result) {
      printf("%s %s 404\n", method, req->path);
      return;
    }
    printf("%s %s 200\n", method, req->path);
  } else {
    Response *response = initResponse();
    addHeader("Connection", "keep-alive", response->headers);
    addHeader("Keep-Alive", "timeout=5, max=100", response->headers);
    addHeader("Content-Type", "text/html", response->headers);
    Route *current_route = hasRoute(req->method, req->path);
    if (current_route != NULL) {
      current_route->callback(req, response);
      printf("%s %s 200\n", method, req->path);
    } else {
      if (req->method != HEAD) {
        setStatus(404, response);
        setResponseBody(not_found, response);
        printf("%s %s 404\n", method, req->path);
      } else {
        current_route = hasRoute(GET, req->path);
        if (current_route) {
          current_route->callback(req, response);
          printf("%s %s 200\n", method, req->path);
        } else {
          setStatus(404, response);
          setResponseBody(not_found, response);
          printf("%s %s 404\n", method, req->path);
        }
      }
    }

    if (sendResponse(&c, response, req->method) < 0) {

      perror("[handleRequest] Sending error");
    }

    freeResponse(&response);
    return;
  }
}
void handleExit(int signum) {
  cleanupRoutes();
  exit(signum);
}
int initializeSocket() {
  signal(SIGINT, handleExit);
#ifdef _WIN32
  WSADATA wsaData;
  int wsa = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (wsa != 0) {
    printf("WSASTARTUP FAILED: %d\n", wsa);
    return -1;
  }
#endif

  setPublicDir("./static/");

  server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == INVALID_SOCKET) {
#ifdef _WIN32
    printf("Unable to initialize socket: %d\n", WSAGetLastError());
    WSACleanup();
#elif defined(__linux__)
    perror("Unable to initialize socket");
#endif    
    return -1;
  }
  int opt = 1;
  int res =
      setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
  if (res == SOCKET_ERROR) {
    closesocket(server);

#ifdef _WIN32
    printf("Unable to set socket option: %d\n", WSAGetLastError());
    WSACleanup();
#elif defined(__linux__)
    perror("Unable to set socket option");
#endif
    return -1;
  }
  return 1;
}

#ifdef _WIN32
void handleClient(LPVOID lpParam) {
  SOCKET client = (SOCKET)lpParam;
#elif defined(__linux__)
void* handleClient(LPVOID lpParam) {
   SOCKET client = (int)(intptr_t)lpParam;
#endif
  uint32_t timeout_ms = 5000;

  setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char *)(&timeout_ms),
             sizeof(timeout_ms));

  while (1) {
    Request *req = initRequest();
    int ret = buildRequest(client, req);
    if (ret == 1) {
      handleRequest(client, req);
    } else {

      freeRequest(&req);
      break;
    }
    freeRequest(&req);
  }
  closesocket(client);
#ifdef __linux__
  return NULL;
#endif
}

void startServer(char *addr, int port) {
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = inet_addr(addr);
  int res = bind(server, (SOCKADDR *)&saddr, sizeof(saddr));
  if (res != 0) {
    closesocket(server);
 #ifdef _WIN32
    printf("Unable to bind socket: %d\n", WSAGetLastError());
    WSACleanup();
#elif defined(__linux__)
    perror("Unable to bind socket");
#endif
    return;
  }
  res = listen(server, MAXCONN);
  if (res != 0) {
    closesocket(server);
#ifdef _WIN32
    printf("Unable to listen socket: %d\n", WSAGetLastError());
    WSACleanup();
#elif defined(__linux__)
    perror("Unable to listen socket");
#endif
    return;
  }

  printf("Server started: http://%s:%d\n", addr, port);
  SOCKET c;
  struct sockaddr_in caddr;
  unsigned int caddr_len = sizeof(caddr);
  while (1) {
    c = accept(server, (SOCKADDR *)&caddr, &caddr_len);
    if (c == INVALID_SOCKET) {
      closesocket(server);
   #ifdef _WIN32
      printf("Unable to accept socket: %d\n", WSAGetLastError());
    WSACleanup();
#elif defined(__linux__)
    perror("Unable to accept socket");

#endif
      return;
    }
#ifdef _WIN32
    HANDLE hndl = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handleClient, (LPVOID)c, 0, NULL);
    if (hndl == NULL) {
        printf("Unable to start thread: %d\n", WSAGetLastError());
#else
    pthread_t tmp;
    int result = pthread_create(&tmp, NULL, handleClient, ((void*)(intptr_t)c));
    if (result != 0){ 
        fprintf(stderr, "Unable to start thread: %s\n", strerror(result));
#endif
        closesocket(c);
        closesocket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

#ifdef __linux__
    pthread_detach(tmp);
#else
    CloseHandle(hndl);   
#endif
    }
}
