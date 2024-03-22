#include <stdbool.h>
#include <semaphore.h>
#include "../flags.h"
#include "../message.h"

#define CONNECTION_TIMEOUT -1
#define MAX_CONNECTIONS 512
#define CONN_QUEUE 5
#define MAX_MESSAGE_LENGTH 2048
#define EMPTY_FD (int)-2

// Error codes
#define NET_SUCCESS 0
#define NET_CHECK_ERRNO -1
#define NET_TIMEDOUT -2
#define NET_AUTH_FAIL -3
#define NET_CONN_BROKE -4
#define NET_INVAL_MSG -5

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
extern int openMainSocket(const int port, int* fd);
extern int closeConnection(conn_t* conn, sem_t* mutex);
extern bool message_is_valid(msg_t* msg, const int size);