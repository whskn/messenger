#include <stdbool.h>
#include "../flags.h"
#include "../message.h"

#define CONNECTION_TIMEOUT 6000 // change
#define CONN_RETRY 2
#define AFK_TIMEOUT -1

// ERROR CODES
#define NET_SUCCESS              0
#define NET_CHECK_ERRNO         -1
#define NET_INVALID_NAME        -2
#define NET_USER_EXISTS         -3
#define NET_SERVER_OVERLOADED   -4
#define NET_SERVER_ERROR        -5
#define NET_CONN_DOWN           -6
#define NET_INVAL_MSG_FORMAT    -7

typedef struct {
    int fd;
    fromto_t addr;
} connection_t;

extern int sendMessage(connection_t* c, msg_t* msg);
extern int readMsg(connection_t* c, msg_t* msg);
extern int clientConnect(connection_t* c, const char* ip, const int port);
extern int closeConn(connection_t* c);