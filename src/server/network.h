#include <semaphore.h>

#include "../flags.h"
#include "../message.h"

#define EMPTY_FD (int)-2

// Error codes
#define NET_SUCCESS 0
#define NET_CHECK_ERRNO -1
#define NET_TIMEDOUT -2
#define NET_AUTH_FAIL -3
#define NET_CONN_BROKE -4
#define NET_INVAL_MSG -5
#define NET_CRITICAL_FAIL -6

typedef struct {
    int fd;
    username_t name;
} conn_t;

typedef struct {
    int id; 
    conn_t* conns;
    sem_t* mutex;
} MC_arg_t;

extern int harvestConnection(const int sockFd);
extern int sendMessage(int fd, msg_t* msg);
extern int authUser(int fd, conn_t* conns, sem_t* mutex);
extern int openMainSocket(const int port);
extern int closeConnection(conn_t* conn, sem_t* mutex);