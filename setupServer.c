#include "include/setupServer.h"
#include "include/httpResponse.h"
void handleRequest(SOCKET c, Request *req) {
  char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found<title/>"
                     "<head/>"
                     "<body>404 NOT FOUND!!<body/>"
                     "<html/>";

  if (exists(req->path)) {

    int result = sendFile(c, req->path);

    if (!result) {
      printf("%s %s 404\n", req->method, req->path);
      return;
    }
    printf("%s %s 200\n", req->method, req->path);
  } else {
    Response *response;
    initResponse(&response);
    addHeader("Content-Type", "text/html", response);
    printf("%s\n", response->headers);
    freeResponse(&response);
    Route *current_route = hasRoute(req->path);
    char *body = NULL;
    if (current_route != NULL) {
      body = current_route->callback(req, response);
    }
    if (body == NULL) {
      int n = strlen(not_found);
      body = malloc(n + 1);
      if (body == NULL)
        return;
      strcpy(body, not_found);
      body[n] = '\0';
    }
    char *resp = (char *)malloc(strlen(body) + 100);
    if (resp == NULL) {
      free(body);
      perror("Error sending data: ");
      return;
    }
    HTMLResponse(resp, body);
    int bytes_sent = send(c, resp, strlen(resp), 0);
    if (bytes_sent == -1) {
      perror("Error sending data: ");
    }
    free(resp);
    free(body);
    if (current_route != NULL) {
      printf("%s %s 200\n", req->method, req->path);
    } else {
      printf("%s %s 404\n", req->method, req->path);
    }
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

  setStaticPath("./static/");

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
  parseRequest(buff, &req);
  handleRequest(client, &req);
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
