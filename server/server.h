#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>

typedef short msg_size;

extern int harvestConnection(const int port, conn_t* connection);

extern int sendMesage(int fd, const char* message, msg_size size);

typedef struct {
    int sockFd;
    int connFd;
} conn_t;