/* Network communication interface */

#ifndef _NETWORK
#define _NETWORK

#include <stdbool.h>
#include "../proto.h"

#define CONNECTION_TIMEOUT 2500
#define AUTH_TIMEOUT 6000
#define CONN_RETRY 2
#define AFK_TIMEOUT -1

// ERROR CODES
#define NET_SUCCESS 0
#define NET_CHECK_ERRNO -1
#define NET_INVALID_AUTH -2
#define NET_USER_EXISTS -3
#define NET_SERVER_OVERLOADED -4
#define NET_SERVER_ERROR -5
#define NET_CONN_DOWN -6
#define NET_INVAL_MSG_FORMAT -7
#define NET_NO_USER -8
#define NET_ERROR -9
#define NET_TIMEOUT -10
#define NET_USER_ONLINE -11

/* stores info about connection to server*/
typedef struct
{
    int fd;
    int my_id;
    username_t my_name;
} connection_t;

/* Connects to the server */
extern int net_connect(connection_t *c, const char *ip, const int port,
                       username_t my_name, password_t password,
                       const bool new_acc);

/* Request user info */
extern int net_user_req(connection_t *c, username_t name);

/* send message of mgs_t format */
extern int net_send_msg(connection_t *c, msg_t *msg);

/* read message */
extern int net_read(const int fd, void *buffer, const int size);

/* close connection with server */
extern int net_close_conn(connection_t *c);

/* flush data on connection fd */
extern int net_flush(connection_t *c);

/* send arbitrary data */
extern int net_send(const int fd, void *buffer, const int size);

extern int net_build_msg(connection_t *c, msg_t *msg, const char *buffer,
                         const int to_id);

#endif