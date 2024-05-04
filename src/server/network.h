/* Header file provides wrapper functions for network communication system
calls*/

// Error codes
#define NET_SUCCESS 0
#define NET_CHECK_ERRNO -1
#define NET_TIMEDOUT -2
#define NET_AUTH_FAIL -3
#define NET_CONN_BROKE -4
#define NET_INVAL_MSG -5
#define NET_CRITICAL_FAIL -6
#define NET_ERROR -7

#define CONN_QUEUE 5

/* Get wait for and get new connection (Blocks runtime) */
extern int net_harvest_conn(const int sockFd);

/* send data */
extern int net_send(const int fd, void *buffer, const int size);

/* recv data */
extern int net_read(const int fd, void *buffer, const int size);

extern int net_open_sock(const int port);
extern int net_close_conn(const int fd);