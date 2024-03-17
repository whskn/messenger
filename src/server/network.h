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
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../flags.h"

#define CONNECTION_TIMEOUT 300000
#define MAX_CONNECTIONS 512
#define CONN_QUEUE 5
#define MAX_MESSAGE_LENGTH 2048
#define EMPTY_FD (int)-2

// sizeof(username_t) * 2 + sizeof(time_t) + sizeof(size_t) + sizeof(char) = 81
#define MIN_MESSAGE_LEN 81

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
    username_t name;
} conn_t;

typedef struct {
    int id; 
    conn_t* conns;
    sem_t* mutex;
} MC_arg_t;

extern int harvestConnection(const int sockFd, int* fd);
extern int sendMessage(int fd, msg_t* msg);
extern int authUser(int fd, int* idptr, conn_t* conns, sem_t* mutex);
extern int openMainSocket(const int port, int* fd);
extern int closeConnection(conn_t* conn, sem_t* mutex);
