#include "include/setupServer.h"

#include "include/connection.h"
#include "include/request.h"
#include "include/response.h"
#include "include/routing.h"
#include <limits.h>
    #include <stdlib.h>
#ifdef _WIN32
#include <winerror.h>
#endif


#define MAX_EVENTS 10


void handleRequest(Connection *con) {
   char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found</title>"
                     "</head>"
                     "<body>404 NOT FOUND!!</body>"
                     "</html>";
    if(con->state != REQUEST_BUILT) return;
    Request * req = con->req;
    const char *method = methods[req->method];
    char public_path[PATH_MAX];
    char req_path[PATH_MAX];
    char resolved_path[PATH_MAX];
    getPublicDir(public_path);
    strncpy(req_path, public_path, PATH_MAX);
    strncat(req_path, req->path, PATH_MAX - strlen(req_path) - 1);

    char *r = realpath(req_path, resolved_path);
        
    if (r != NULL && strncmp(resolved_path, public_path, strlen(public_path)) == 0 && exists(req_path)) {
            printf("GET %s 200 \n", req->path);
            int result = sendFile(con,req_path);
            if (result < 0) {
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

    if (sendResponse(con) < 0) {

      perror("[handleRequest] Sending error");
    }
    con->res = response;
    freeResponse(&con->res);
    response = NULL;
    return;
  }
}
void handleExit(int signum) {
  cleanupRoutes();
  exit(signum);
}



int set_nonblocking(int sockfd) {
    
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    
    //TODO: Add Nonblocking for windows(ioctl).
    
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

void print_error(char* error_msg){
#ifdef _WIN32
    printf("%s: %d\n",error_msg, WSAGetLastError());
    WSACleanup();
#elif defined(__linux__)
    perror(error_msg);
#endif


}

int setup_async(SOCKET server_fd){
#ifdef _WIN32
    //TODO: epoll for windows?
#elif defined(__linux__)
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }
    struct epoll_event event;
    event.events = EPOLLIN;     
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl: server_fd");
        return -1;
    }
#endif
    return epoll_fd;

}




int startServer(char *addr, int port) {
    signal(SIGINT, handleExit);


#ifdef _WIN32
    WSADATA wsaData;
    int wsa = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa != 0) {
        printf("WSASTARTUP FAILED: %d\n", wsa);
        return -1;
    }
#endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        print_error("Unable to initialize socket");
        return -1;
    }
    int opt = 1;
    int res = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (res == SOCKET_ERROR) {
        closesocket(server_fd);
        print_error("Unable to set socket option");
        return -1;
    }
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr(addr);
    res = bind(server_fd, (SOCKADDR *)&saddr, sizeof(saddr));
    
    if (res < 0) {
        print_error("Unable to bind socket");
        printf("server_fd : %d\n",server_fd);
        closesocket(server_fd);
        return -1;
    }

    set_nonblocking(server_fd);
    res = listen(server_fd, MAXCONN);

    if (res != 0) {
        closesocket(server_fd);
        print_error("Unable to listen socket");
        return -1;
    }
    int epoll_fd = setup_async(server_fd);
    if(epoll_fd < 0){
        return -1;
    }
    SOCKET c;
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
            print_error("epoll_wait");
            break;
        }
        for (int i = 0; i < n_events; i++) {
            struct epoll_event event = events[i];
            if(event.data.fd == server_fd){
                    while(1){
                        SOCKADDR_IN caddr;
                        unsigned int caddr_len = sizeof(caddr);
                        SOCKET client_fd = accept(server_fd,(SOCKADDR*)&caddr,&caddr_len);
                        if(client_fd == -1){
                            int e = GET_NET_ERROR;
                            if(IS_WOULD_BLOCK(e)){
                                break;
                            }else{
                                print_error("Error in accept");
                                break;
                            }
                        }
                        set_nonblocking(client_fd);
                        Connection * con = init_connection();
                        con->state = PARSING_HEADERS; 
                        struct epoll_event new_event; 
                        new_event.events = EPOLLIN|EPOLLONESHOT;     
                        new_event.data.ptr = con;
                        con->client = client_fd; 
                        if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&new_event) == -1){
                            print_error("epoll_ctl: client_fd");
                            free_connection(&con);
                        }
        
                    }
            }else{
                Connection* con = (Connection*)event.data.ptr;
                if(event.events & EPOLLIN){
                        if(con->state != REQUEST_BUILT){
                            int e = build_request(con);
                            if(e == ERR_CONTENT_TOO_LARGE){
                                event.events = EPOLLOUT | EPOLLONESHOT;
                                event.data.ptr = con;
                                if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
                                    print_error("epoll_ctl: client_fd");
                                    free_connection(&con);
                                    continue;
                                }
                            }
                            if(e<0){
                                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,con->client,&event);
                                free_connection(&con); 
                            }
                        }else{
                            handleRequest(con);
                            event.events = EPOLLOUT | EPOLLONESHOT;
                            event.data.ptr = con;
                            if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
                                    print_error("epoll_ctl: client_fd");
                                    free_connection(&con);
                            }
                        }
                        

                }
                if (event.events & EPOLLRDHUP){
                       epoll_ctl(epoll_fd,EPOLL_CTL_DEL,con->client,&event);
                         free_connection(&con);        
                }
                if (event.events & EPOLLOUT){
                        if(con->state == SENDING_FILE || con->state == SENDING_FILE_HEADERS){
                            sendFile(con,NULL);    
                        }
                        if(con->state == SENDING_RESPONSE){
                            sendResponse(con);
                        }
                        if(con->state == RESPONSE_SENT){
                            freeRequest(&con->req);
                            freeResponse(&con->res);
                            memset(&con->data, 0, sizeof(con->data)); 
                            con->state = PARSING_HEADERS;
                            event.events = EPOLLIN | EPOLLONESHOT;
                            event.data.ptr = con;
                            if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
                                print_error("epoll_ctl: client_fd");
                                free_connection(&con);
                            }                    

                        } 
                }
                 

            }         
        }
    }
#ifdef _WIN32
        WSACleanup();
#endif

}
