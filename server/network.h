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

#define CODE_SIZE 9
#define HANDSHAKE_CODE "HANDSHAKE"
#define HANDSHAKE_SUCCESS "HS_SUCC"
#define HANDSHAKE_FAIL "HS_FAIL"

typedef short msg_size_t;
typedef char username_t[64];

typedef struct {
    int sockFd;
    int connFd;
} conn_t;

typedef struct {
    conn_t* conn; 
    username_t* usernames; 
    conn_t* connections;
} MC_arg_t;

extern int harvestConnection(const int port, 
                             conn_t* connection, 
                             conn_t* connections, 
                             username_t* names);

extern int sendMessage(int fd, const char* message, msg_size_t size);

extern void* manageConnection(void* args);

extern int authUser(conn_t conn, username_t username);

