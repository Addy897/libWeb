#include "setup_server.h"
#include "globals.h"
#include "compat.h"
#include "connection.h"
#include "request.h"
#include "response.h"
#include "routing.h"
#include <limits.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/sysinfo.h> 
#ifdef _WIN32
	#include <winerror.h>
#endif

#define MAXCONN 8192
#define BUFF_SIZE 1024
#define CONN_READ_FLAGS  (EPOLLIN  | EPOLLONESHOT | EPOLLET | EPOLLRDHUP)
#define CONN_WRITE_FLAGS (EPOLLOUT | EPOLLONESHOT | EPOLLET)
#define MAX_EVENTS 4096
SOCKET server = INVALID_SOCKET;
typedef struct {
    char *addr;
    int port;
    int thread_id;
} server_worker_arg_t;
__thread CacheStore FILE_CACHE;

volatile atomic_int keep_running = 1;
void handleRequest(Connection *con,HashTable * cache) {
   char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found</title>"
                     "</head>"
                     "<body>404 NOT FOUND!!</body>"
                     "</html>";
    if(con->state != REQUEST_BUILT) return;
    if(cache!=NULL && con->req->method == GET){
        StringView content = get_as_sv(con->req->path,cache);
        if(!sv_eq(content,SV_NULL)){
               con->data.res.resp_buf = content.data;
               con->data.res.total_size = content.count;
               con->state = SENDING_RESPONSE;
               return;
            }        


    }
    Request * req = con->req;
    const char *method = methods[req->method];
    char req_path[PATH_MAX];
    char resolved_path[PATH_MAX];
    int req_path_len = PUBLIC_DIR_LEN + req->path.count;
    memcpy(req_path, PUBLIC_DIR, PUBLIC_DIR_LEN);
    memcpy(req_path + PUBLIC_DIR_LEN, req->path.data, req->path.count);
    req_path[req_path_len] = '\0';
    char *r = realpath(req_path, resolved_path);
    //if(0){  
    if (r != NULL && strncmp(resolved_path, PUBLIC_DIR, PUBLIC_DIR_LEN) == 0 && exists(req_path)) {
        con->state = SENDING_FILE_HEADERS;
        memcpy(con->file.filepath,req_path,req_path_len);
        con->file.filepath[req_path_len] = '\0';
        printf("%s "SV_Fmt" 200\n", method, SV_Arg(req->path));
  } else {
        con->res = initResponse();
        Response* response = con->res;
        add_response_header("connection", "keep-alive", response);
        add_response_header("keep-alive", "timeout=5, max=100", response);
        add_response_header("content-type", "text/html", response);
        Route *current_route = hasRoute(req->method, req->path);
        if (current_route != NULL) {
          current_route->callback(req, response);
          //printf("%s "SV_Fmt" 200\n", method, SV_Arg(req->path));
        } else {
            if (req->method != HEAD) {
                setStatus(404, response);
                set_response_body_sv(sv_from_cltr(not_found), response);
                //printf("%s "SV_Fmt" 404\n", method, SV_Arg(req->path));
            } else {
                current_route = hasRoute(GET, req->path);
                if (current_route) {
                    current_route->callback(req, response);
                    //printf("%s "SV_Fmt" 200\n", method, SV_Arg(req->path));
                } else {
                    setStatus(404, response);
                    set_response_body_sv(sv_from_cltr(not_found), response);
                    //printf("%s "SV_Fmt" 404\n", method, SV_Arg(req->path));
                }
            }
        }
        con->state = SENDING_RESPONSE;
    }
}
void handle_exit(int signo, siginfo_t *info, void *context) {
    atomic_store(&keep_running,0);
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
    struct sigaction sa;
    
    sa.sa_sigaction = handle_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO; 

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }
    
    HashTable * cache = init_table(16);

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
    while (atomic_load(&keep_running)) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS,1000);
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
                            handleRequest(con,cache);
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
                            e = sendResponse(con,cache);
                        }
                        if (e == 0) {
                                event.events = CONN_WRITE_FLAGS;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->client, &event);
                            }
                        if(con->state == RESPONSE_SENT){
                            freeRequest(&con->req);
                            if(con->res !=NULL)
                            freeResponse(&con->res);
                            con->data.req.pos = 0;
                            con->data.res.total_size = 0;
                            con->data.res.bytes_sent = 0;
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
    if (!cache)
        return 0;
    if (cache->entries != NULL) {
        for (int i = 0; i < cache->capacity; i++) {
            HashEntry *current = cache->entries[i];
            while (current != NULL) {
                HashEntry *temp = current;
                current = current->next;
                if(temp->owns_key)
                    free(temp->key.data);
                temp->key.count = 0;
                temp->key.data = NULL;
                free(((StringView *)temp->value)->data);
                free(temp->value);
                free(temp);
            }
        }
    }
    free(cache->entries);
    free(cache);
    cache = NULL;

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
    printf("Connection %d\n",sizeof(Connection));
    FILE_CACHE = init_cache_store();
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
    free_cache_items(&FILE_CACHE);
    cleanupRoutes();
    return 0;
}

