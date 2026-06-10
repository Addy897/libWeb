#include "include/setupServer.h"

#include "include/connection.h"
#include "include/request.h"
#include "include/response.h"
#include "include/routing.h"
#include <limits.h>
#include <stdlib.h>

#include <sys/sysinfo.h> 

#ifdef _WIN32
	#include <winerror.h>
#endif

#define CONN_READ_FLAGS  (EPOLLIN  | EPOLLONESHOT | EPOLLET | EPOLLRDHUP)
#define CONN_WRITE_FLAGS (EPOLLOUT | EPOLLONESHOT | EPOLLET)
#define MAX_EVENTS 4096

typedef struct {
    char *addr;
    int port;
    int thread_id;
} server_worker_arg_t;

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
    snprintf(req_path, sizeof(req_path), "%s"SV_Fmt"", public_path, SV_Arg(req->path));
    char *r = realpath(req_path, resolved_path);
        
    if (r != NULL && strncmp(resolved_path, public_path, strlen(public_path)) == 0 && exists(req_path)) {
        con->state = SENDING_FILE_HEADERS;
        strncpy(con->file.filepath,req_path,strlen(req_path));
        printf("%s "SV_Fmt" 200\n", method, SV_Arg(req->path));
  } else {
        con->res = initResponse();
        Response* response = con->res;
        addHeader("Connection", "keep-alive", response->headers);
        addHeader("Keep-Alive", "timeout=5, max=100", response->headers);
        addHeader("Content-Type", "text/html", response->headers);
        Route *current_route = hasRoute(req->method, req->path);
        if (current_route != NULL) {
          current_route->callback(req, response);
          printf("%s "SV_Fmt" 200\n", method, SV_Arg(req->path));
        } else {
            if (req->method != HEAD) {
                setStatus(404, response);
                setResponseBody(not_found, response);
                printf("%s "SV_Fmt" 404\n", method, SV_Arg(req->path));
            } else {
                current_route = hasRoute(GET, req->path);
                if (current_route) {
                    current_route->callback(req, response);
                    printf("%s "SV_Fmt" 200\n", method, SV_Arg(req->path));
                } else {
                    setStatus(404, response);
                    setResponseBody(not_found, response);
                    printf("%s "SV_Fmt" 404\n", method, SV_Arg(req->path));
                }
            }
        }
        con->state = SENDING_RESPONSE;
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




int ev_loop(char* addr,int port) {
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
    res = setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (res == SOCKET_ERROR) {
        closesocket(server_fd);
        print_error("Unable to set socket option");
        return -1;
    }
    SOCKADDR_IN saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    if(inet_pton(AF_INET, addr, &saddr.sin_addr) <= 0){
        print_error("Invalid address");
        return -1;
    }    
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
                        new_event.events = CONN_READ_FLAGS;    
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
                            if(con->req == NULL){
                                con->req = initRequest();
                            }
                            int e = build_request(con);
                            if(e == ERR_CONTENT_TOO_LARGE){
                                event.events = CONN_WRITE_FLAGS;
                                event.data.ptr = con;
                                if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
                                    print_error("epoll_ctl: client_fd");
                                    free_connection(&con);
                                }
                                continue;
                            }else if (e == 0) {
                                event.events = CONN_READ_FLAGS;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->client, &event);
                            }
                            if(e<0){
                                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,con->client,&event);
                                free_connection(&con); 
                                continue;
                            }
                        }
                        if(con->state == REQUEST_BUILT){
                            handleRequest(con);
                            event.events = CONN_WRITE_FLAGS;
                            if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
                                    print_error("epoll_ctl: client_fd");
                                    free_connection(&con);
                                    continue;
                            }
                        }
                        

                }
                if (event.events & EPOLLRDHUP){
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,con->client,&event);
                    free_connection(&con);
                    continue;       
                }
                if (event.events & EPOLLOUT){
                        int e = 0;
                        if(con->state == SENDING_FILE || con->state == SENDING_FILE_HEADERS){
                            e = sendFile(con);    
                        }
                        if(con->state == SENDING_RESPONSE){
                            e = sendResponse(con);
                        }
                        if (e == 0) {
                                event.events = CONN_WRITE_FLAGS;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->client, &event);
                            }
                        if(con->state == RESPONSE_SENT){
                            freeRequest(&con->req);
                            freeResponse(&con->res);
                            memset(&con->data, 0, sizeof(con->data)); 
                            memset(&con->file, 0, sizeof(con->file)); 
                            con->state = PARSING_HEADERS;
                            event.events = CONN_READ_FLAGS;
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
void* server_thread_worker(void* arg) {
    server_worker_arg_t *config = (server_worker_arg_t*)arg;
    
    printf("Worker thread %d starting on port %d...\n", config->thread_id, config->port);
    ev_loop(config->addr, config->port);
    
    return NULL;
}

int startServer(char *addr, int port) {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Detected %d CPU cores. Spawning workers...\n", num_cores);

    pthread_t threads[num_cores];
    server_worker_arg_t args[num_cores];

    for (int i = 0; i < num_cores; i++) {
        args[i].addr = addr;
        args[i].port = port;
        args[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, server_thread_worker, &args[i]) != 0) {
            perror("Failed to create thread");
            return -1;
        }
    }

    // 2. Wait for all threads (they will run forever)
    for (int i = 0; i < num_cores; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

