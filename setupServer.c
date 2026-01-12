#include "include/setupServer.h"
#include "include/httpResponse.h"
#include "include/routing.h"
void handleRequest(SOCKET c, Request *req) {
  char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found</title>"
                     "</head>"
                     "<body>404 NOT FOUND!!</body>"
                     "</html>";

  if (exists(req->path)) {

    int result = sendFile(c, req->path);

    if (!result) {
      printf("%s %s 404\n", req->method, req->path);
      return;
    }
    printf("%s %s 200\n", req->method, req->path);
  } else {
    Response *response = initResponse();
    addHeader("Content-Type", "text/html", response);
    const char *header = getHeader("Content-Type", response);
    Route *current_route = hasRoute(req->path);
    if (current_route != NULL) {
      current_route->callback(req, response);
      printf("%s %s 200\n", req->method, req->path);
    } else {
      setStatus(404, response);
      setBody(not_found, response);
      printf("%s %s 404\n", req->method, req->path);
    }

    if (sendResponse(&c, response) < 0) {
      perror("Sending Error");
    }

    freeResponse(&response);
    return;
  }
}
void handleExit(int signum) {
  cleanupRoutes();
  exit(signum);
}
int _startServer(SOCKET server, SOCKET c, SOCKADDR_IN *caddr) {
  char *str = inet_ntoa(caddr->sin_addr);

  char recvbuf[1024];
  int recvbuflen = 1024;
  int res = recv(c, recvbuf, recvbuflen, 0);
  if (res > 0) {
    Request req;
    parseRequest(recvbuf, &req);
  }
  return 0;
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
  char buff[BUFF_SIZE];

  int result = recv(client, buff, BUFF_SIZE, 0);
  if (result < 0) {
    closesocket(client);
    return;
  }
  Request req;
  if (parseRequest(buff, &req)) {
    handleRequest(client, &req);
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
      printf("Unable to accept socket: %d\n", WSAGetLastError());
      closesocket(c);
      closesocket(server);
      WSACleanup();
      return;
    }
  }
}
