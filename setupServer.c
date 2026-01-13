#include "include/setupServer.h"
#include "include/request.h"
#include "include/response.h"
#include "include/routing.h"
#include <winerror.h>
#include <winsock2.h>
void handleRequest(SOCKET c, Request *req) {
  char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found</title>"
                     "</head>"
                     "<body>404 NOT FOUND!!</body>"
                     "</html>";
  const char *method = methods[req->method];
  if (exists(req->path)) {

    int result = sendFile(c, req->path);

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
      setStatus(404, response);
      setResponseBody(not_found, response);
      printf("%s %s 404\n", method, req->path);
    }

    if (sendResponse(&c, response) < 0) {

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

  WSADATA wsaData;
  int wsa = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (wsa != 0) {
    printf("WSASTARTUP FAILED: %d\n", wsa);
    return -1;
  }

  setPublicDir("./static/");

  server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server == INVALID_SOCKET) {
    printf("Unable to initialize socket: %d\n", WSAGetLastError());
    WSACleanup();
    return -1;
  }
  int opt = 1;
  int res =
      setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
  if (res == SOCKET_ERROR) {
    printf("Unable to set socket option: %d\n", WSAGetLastError());
    closesocket(server);
    WSACleanup();
    return -1;
  }
  return 1;
}
void handleClient(LPVOID lpParam) {

  SOCKET client = (SOCKET)lpParam;
  DWORD timeout_ms = 5000;

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
}

void startServer(char *addr, int port) {
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = inet_addr(addr);
  int res = bind(server, (SOCKADDR *)&saddr, sizeof(saddr));
  if (res != 0) {
    printf("Unable to bind socket: %d\n", WSAGetLastError());
    closesocket(server);
    WSACleanup();
    return;
  }
  res = listen(server, MAXCONN);
  if (res != 0) {
    printf("Unable to bind socket: %d\n", WSAGetLastError());
    closesocket(server);
    WSACleanup();
    return;
  }

  printf("Server started: http://%s:%d\n", addr, port);
  SOCKET c;
  struct sockaddr_in caddr;
  int caddr_len = sizeof(caddr);
  while (1) {
    c = accept(server, (SOCKADDR *)&caddr, &caddr_len);
    if (c == INVALID_SOCKET) {
      printf("Unable to accept socket: %d\n", WSAGetLastError());
      closesocket(server);
      WSACleanup();
      return;
    }
    HANDLE hndl = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handleClient,
                               (LPVOID)c, 0, NULL);
    if (hndl == NULL) {
      printf("Unable to start thread: %d\n", WSAGetLastError());
      closesocket(c);
      closesocket(server);
      WSACleanup();
      return;
    }
  }
}
