#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include "../flags.h"
#include "../message.h"

#define CONNECTION_TIMEOUT 6000 // change
#define CONN_RETRY 2
#define AFK_TIMEOUT -1

typedef struct {
    int fd;
    fromto_t addr;
    msg_t* msg;
} connection_t;

extern int sendMessage(connection_t* c, char* buffer, size_t length);
extern int readMsg(connection_t* c);
extern int clientConnect(connection_t* c, const char* ip, const int port);
extern int closeConn(connection_t* c);