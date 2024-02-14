#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/time.h>


#define MAX_MESSAGE_LENGTH 1024
#define CONNECTION_TIMEOUT 30000
#define MAX_CONNECTIONS 512

typedef short msg_size_t;
typedef char username_t[64];

extern int harvestConnection(const int port, conn_t* connection);

extern int sendMessage(int fd, const char* message, msg_size_t size);

extern int manageConnection(conn_t conn, username_t* usernames, conn_t* connections);

typedef struct {
    int sockFd;
    int connFd;
} conn_t;
