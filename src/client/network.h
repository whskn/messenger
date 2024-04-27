#include <stdbool.h>
#include "../message.h"

#ifndef _NETWORK
# define _NETWORK

#define CONNECTION_TIMEOUT 2500
#define AUTH_TIMEOUT 6000
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
#define NET_NO_USER             -8
#define NET_ERROR               -9
#define NET_TIMEOUT             -10

typedef struct {
    int fd;
    int my_id;
    username_t my_name;
} connection_t;

extern int net_connect(connection_t* c, const char* ip, const int port, 
                       username_t my_name, password_t password, 
                       const bool new_acc);
extern int net_user_req(connection_t* c, username_t name);
extern int net_send_msg(connection_t* c, msg_t* msg, char* buffer, const int to_id);
extern int net_read(const int fd, void* buffer, const int size);
extern int net_close_conn(connection_t* c);
extern int peak_int(connection_t* c);
extern int net_flush(connection_t* c);
extern int net_send(const int fd, void* buffer, const int size);

#endif