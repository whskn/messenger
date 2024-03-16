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

#define CONNECTION_TIMEOUT 6000 // change
#define CONN_RETRY 2
#define AFK_TIMEOUT -1

typedef int hs_code_t;

#define MAX_MESSAGE_LENGTH 2048

// sizeof(username_t) * 2 + sizeof(time_t) + sizeof(size_t) + sizeof(char) = 81
#define MIN_MESSAGE_LEN 81

typedef char username_t[32];

typedef struct {
    username_t from;
    username_t to;
} fromto_t;

typedef struct {
    fromto_t names;
    time_t timestamp;
    size_t msg_size;
    char buffer[MAX_MESSAGE_LENGTH];
} msg_t;

typedef struct {
    int fd;
    fromto_t addr;
    msg_t* msg;
} connection_t;

extern int sendMessage(connection_t* c, char* buffer, size_t length);
extern int readMsg(connection_t* c);
extern int clientConnect(connection_t* c, const char* ip, const int port);
extern int closeConn(connection_t* c);