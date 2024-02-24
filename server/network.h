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

#define MAX_MESSAGE_LENGTH 1024
#define CONNECTION_TIMEOUT 30000
#define MAX_CONNECTIONS 512
#define CONN_QUEUE 5

// HS CODES
#define HS_SUCC (char)0b101
#define HS_MAX_CONN (char)0b001
#define HS_INVAL_NAME (char)0b010
#define HS_USER_EXISTS (char)0b100

#define EMPTY_FD (int)-2

typedef short msg_size_t;
typedef char username_t[64];

typedef struct {
    int fd;
    username_t name;
} conn_t;

typedef struct {
    int* fd; 
    conn_t* conns;
    sem_t* mutex;
} MC_arg_t;

extern int harvestConnection(const int sockFd);
extern int sendMessage(int fd, const char* message, msg_size_t size);
extern void* manageConnection(void* args);
extern int authUser(int fd, conn_t* conns, sem_t* mutex);
extern int openMainSocket(const int port);