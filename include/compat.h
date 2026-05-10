#ifndef COMPAT_H
#define COMPAT_H
#ifdef _WIN32

    #include <io.h>
    #define GET_NET_ERROR (WSAGetLastError())
    #define IS_WOULD_BLOCK(x) ((x) == WSAEWOULDBLOCK)

#elif defined(__linux__)

  #include <pthread.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <errno.h>
  #include <netinet/in.h>
  typedef void* LPVOID;
  #define closesocket close
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
  #define SOCKADDR        struct sockaddr
  #define SOCKADDR_IN     struct sockaddr_in
  #define GET_NET_ERROR (errno)
  #define IS_WOULD_BLOCK(x) ((x) == EAGAIN || (x) == EWOULDBLOCK)

#endif
#endif

