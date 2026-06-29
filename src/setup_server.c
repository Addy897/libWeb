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
#include <sys/eventfd.h> 
#ifdef _WIN32
	#include <winerror.h>
#endif

#define MAXCONN 1024
#define MAX_SIZE 1024

#define MAX_WORKERS 4


#define BUFF_SIZE 1024
#define CONN_READ_FLAGS  (EPOLLIN  | EPOLLONESHOT | EPOLLET | EPOLLRDHUP)
#define CONN_WRITE_FLAGS (EPOLLOUT | EPOLLONESHOT | EPOLLET)
#define MAX_EVENTS 4096
SOCKET server = INVALID_SOCKET;
int master_exit_fd;

__thread CacheStore FILE_CACHE;



typedef struct {
    int fds[MAX_SIZE];
    atomic_int head;
    atomic_int tail;
    int wake_fd;
    int exit_fd;
}FD_Queue;

static FD_Queue FD_QUEUES[MAX_WORKERS] ={0};

typedef struct {
    int worker_id;
}worker_arg_t;

typedef struct {
    char *addr;
    int port;
} master_arg_t;






volatile atomic_int keep_running = 1;





int qpush(int worker_id, int fd)
{
    FD_Queue *q = &FD_QUEUES[worker_id];

    int tail = atomic_load(&q->tail);
    int head = atomic_load(&q->head);
    int next_tail = (tail + 1) % MAX_SIZE;

    if (next_tail == head)
        return -1;

    q->fds[tail] = fd;
    atomic_store(&q->tail,next_tail);

    return 1;
}

int qpop(int worker_id)
{
    FD_Queue *q = &FD_QUEUES[worker_id];
    int tail = atomic_load(&q->tail);
    int head = atomic_load(&q->head);

    if (head == tail)
        return -1;

    int fd = q->fds[q->head];
    head = (head + 1) % MAX_SIZE;
    atomic_store(&q->head,head);

    return fd;
}





void handleRequest(Connection *con,HashTable * cache) {
   char not_found[] = "<html>"
                     "<head>"
                     "<title>Not Found</title>"
                     "</head>"
                     "<body>404 NOT FOUND!!</body>"
                     "</html>";
    if(con->state != REQUEST_BUILT) return;
    if(cache!=NULL && con->req.method == GET){
        StringView content = get_as_sv(con->req.path,cache);
        if(!sv_eq(content,SV_NULL)){
               con->data.res.resp_buf = content.data;
               con->data.res.total_size = content.count;
               con->state = SENDING_RESPONSE;
               return;
            }        


    }
    Request * req = &con->req;
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
void handle_exit(int signo) {
    atomic_store(&keep_running,0);
    for( int i=0;i<MAX_WORKERS;i++){
        uint64_t wake = 1; 
        write(FD_QUEUES[i].exit_fd,&wake,sizeof(wake)); 
    }
    uint64_t wake = 1; 
    write(master_exit_fd,&wake,sizeof(wake)); 
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
    master_exit_fd = eventfd(0,EFD_NONBLOCK);
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

static inline int handle_read(struct epoll_event event,HashTable* cache, int epoll_fd){
    Connection* con = (Connection*)event.data.ptr;
    if(con->state != REQUEST_BUILT){
        int e = build_request(con);
        if(e == ERR_CONTENT_TOO_LARGE){
            event.events = CONN_WRITE_FLAGS;
            event.data.ptr = con;
            if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
                print_error("epoll_ctl: client_fd");
                return -1;
            }
            return 0;
        }else if (e == 0) {
            event.events = CONN_READ_FLAGS;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->client, &event);
        }else if(e<0){
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,con->client,&event);
            return 0;
        }
    }
    if(con->state == REQUEST_BUILT){
        handleRequest(con,cache);
        event.events = CONN_WRITE_FLAGS;
        if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,con->client,&event) == -1){
            print_error("epoll_ctl: client_fd");
            return 0;
        }
    }
    return 1;


}

static inline int handle_write(struct epoll_event event,HashTable * cache ,int epoll_fd){
    Connection* con = (Connection*)event.data.ptr;
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
            return 0;
        }
    if(con->state == RESPONSE_SENT){
        con->req.path = SV_NULL;
        con->req.body = SV_NULL;
        con->req.version = SV_NULL;
        con->req.headers.count = 0;
        con->req.query_params.count = 0;
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
            return -1;
        }                    

    }   
    return 1;

}

static inline void consume_fds(int epoll_fd,Connection ** active_con,int * active_count,Connection ** closed_con,int * closed_count,int worker_id){
    int fd;
    while((fd = qpop(worker_id))!=-1){
        if(*closed_count == 0){ 
            Connection * con = init_connection();
            con->state = PARSING_HEADERS; 
            struct epoll_event new_event; 
            new_event.events = CONN_READ_FLAGS;    
            new_event.data.ptr = con;
            con->client = fd; 
            if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&new_event) == -1){
                print_error("epoll_ctl: client_fd");
                closed_con[(*closed_count)++] = con;
                continue;  
            }
            con->index = *active_count;
            active_con[(*active_count)++] = con;
        }else{
            Connection * con = closed_con[--*(closed_count)];
            closed_con[*closed_count] = NULL;
            con->state = PARSING_HEADERS; 
            struct epoll_event new_event; 
            new_event.events = CONN_READ_FLAGS;    
            new_event.data.ptr = con;
            con->client = fd; 
            if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&new_event) == -1){
                print_error("epoll_ctl: client_fd");
                closed_con[(*closed_count)++] = con;
                continue;  
            }
            con->index = *active_count;
            active_con[(*active_count)++] = con;

        }

    }

}

int ev_loop(int worker_id) {
    struct sigaction sa;
    
    sa.sa_handler = handle_exit;
    sa.sa_flags = 0; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }
   HashTable * cache = init_table(16);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }
    FD_Queue * q = &FD_QUEUES[worker_id];
        struct epoll_event ev,ev2;
    ev.events = EPOLLIN;
    ev.data.fd = q->wake_fd;
    ev2.events = EPOLLIN;
    ev2.data.fd = q->exit_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, q->wake_fd, &ev);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, q->exit_fd, &ev2);
    struct epoll_event events[MAX_EVENTS];
    Connection *active_cons[MAXCONN];
    Connection *closed_cons[MAXCONN];
    int active_count = 0;
    int closed_count = 0;
   consume_fds(epoll_fd,active_cons,&active_count,closed_cons,&closed_count,worker_id);
   while (atomic_load(&keep_running)) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS,-1);
        if (n_events == -1) {
            if (errno == EINTR) continue;
            print_error("epoll_wait");
            break;
        }
        for (int i = 0; i < n_events; i++) {
            struct epoll_event event = events[i];
            if(event.data.fd == q->wake_fd){
                uint64_t res;
                read(q->wake_fd, &res, sizeof(uint64_t));
                if(res > 0){
                    consume_fds(epoll_fd,active_cons,&active_count,closed_cons,&closed_count,worker_id);
                }         
                continue;   
            }else if(event.data.fd == q->exit_fd){

                uint64_t res;
                read(q->exit_fd, &res, sizeof(uint64_t));
                if(res > 0){
                    break;
                }
            }
            Connection* con = (Connection*)event.data.ptr;
            if(event.events & EPOLLIN){
                int e = handle_read(event,cache, epoll_fd); 
                if(e == -1){
                    int removed = con->index;
                    Connection *moved = active_cons[--active_count];
                    moved->req.body = SV_NULL;
                    active_cons[removed] = moved;
                    moved->index = removed;
                    closed_cons[closed_count++] = con;                 
                    continue;
                }else if(!e) continue;
                

            }
            if (event.events & EPOLLRDHUP){
                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,con->client,&event);
                if(con->client != -1)
                    close(con->client);                  
                con->client = -1;
                int removed = con->index;
                Connection *moved = active_cons[--active_count];
                moved->req.body = SV_NULL;
                active_cons[removed] = moved;
                moved->index = removed;
                closed_cons[closed_count++] = con;
                continue;       
            }
            if (event.events & EPOLLOUT){
                int e = handle_write(event,cache,epoll_fd); 
                if(e == -1){
                    int removed = con->index;
                    Connection *moved = active_cons[--active_count];
                    moved->req.body = SV_NULL;
                    active_cons[removed] = moved;
                    moved->index = removed;
                    closed_cons[closed_count++] = con;
                    continue;
                }else if(!e) continue;

            }

        }
    }
    done:
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
    for(int i = 0; i<active_count;i++){
        if(active_cons[i]) free_connection(&active_cons[i]);
    }
    for(int i = 0; i<closed_count;i++){
        if(closed_cons[i]) free_connection(&closed_cons[i]);
    }
    printf("Successfully Freed\n");
}
void* worker_thread(void* arg) {
    
    FILE_CACHE = init_cache_store();
    worker_arg_t *config = (worker_arg_t*)arg;
    printf("Worker thread %d is starting....\n", config->worker_id);
    ev_loop(config->worker_id);
    free_cache_items(&FILE_CACHE);
    return NULL;
}


void* master_thread(void * arg){
    struct sigaction sa;
    sa.sa_handler = handle_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        return NULL;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction failed");
        return NULL;
    }
    master_arg_t *config = (master_arg_t*)arg;
    char* addr= config->addr;
    int port = config->port;
#ifdef _WIN32
    WSADATA wsaData;
    int wsa = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa != 0) {
        printf("WSASTARTUP FAILED: %d\n", wsa);
        return NULL;
    }
#endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        print_error("Unable to initialize socket");
        return NULL;
    }
    int opt = 1;
    int res = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    res = setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (res == SOCKET_ERROR) {
        closesocket(server_fd);
        print_error("Unable to set socket option");
        return NULL;
    }
    SOCKADDR_IN saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    if(inet_pton(AF_INET, addr, &saddr.sin_addr) <= 0){
        print_error("Invalid address");
        return NULL;
    }    
    res = bind(server_fd, (SOCKADDR *)&saddr, sizeof(saddr));
    
    if (res < 0) {
        print_error("Unable to bind socket");
        printf("server_fd : %d\n",server_fd);
        closesocket(server_fd);
        return NULL;
    }

    set_nonblocking(server_fd);
    res = listen(server_fd, MAXCONN);

    if (res != 0) {
        closesocket(server_fd);
        print_error("Unable to listen socket");
        return NULL;
    }
    int epoll_fd = setup_async(server_fd);
    if(epoll_fd < 0){
        return NULL;
    }
    struct epoll_event events[MAX_EVENTS],ev;
    ev.events = EPOLLIN;
    ev.data.fd = master_exit_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, master_exit_fd, &ev);

    int current_worker_id = 0;
    while (atomic_load(&keep_running)) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS,-1);
        if (n_events == -1) {
            if (errno == EINTR) continue;
            print_error("epoll_wait");
            break;
        }
        for (int i = 0; i < n_events; i++) {
            struct epoll_event event = events[i];
            if(event.data.fd == master_exit_fd){

                uint64_t res;
                read(master_exit_fd, &res, sizeof(uint64_t));
                if(res > 0){
                    break;
                }

            }
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
                while(qpush(current_worker_id,client_fd)<0){ 
                    current_worker_id = (current_worker_id + 1) % MAX_WORKERS;
                }
                uint64_t wake = 1; 
                write(FD_QUEUES[current_worker_id].wake_fd,&wake,sizeof(wake));       
                current_worker_id = (current_worker_id + 1) % MAX_WORKERS;
            }
        }
    }   
    return NULL;

}


int startServer(char *addr, int port) {
    printf("Connection %d\n",sizeof(Connection));
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Detected %d CPU cores. Spawning workers...\n", num_cores);
    pthread_t threads[MAX_WORKERS+1];
    worker_arg_t args[MAX_WORKERS];
    master_arg_t master_args;
    master_args.addr = addr;
    master_args.port = port;
    for(int i = 0;i<MAX_WORKERS;i++){
        FD_QUEUES[i].wake_fd = eventfd(0,EFD_NONBLOCK);
        FD_QUEUES[i].exit_fd = eventfd(0,EFD_NONBLOCK);
    }    

    
    if (pthread_create(&threads[0], NULL, master_thread, &master_args) != 0) {
            perror("Failed to create thread");
            return -1;
    }

    for (int i = 0; i < MAX_WORKERS; i++) {
        args[i].worker_id = i;
        if (pthread_create(&threads[i+1], NULL, worker_thread, &args[i]) != 0) {
            perror("Failed to create thread");
            return -1;
        }
    }

    // 2. Wait for all threads (they will run forever)
    for (int i = 0; i < MAX_WORKERS+1; i++) {
        pthread_join(threads[i], NULL);
    }   
    return 0;
}

