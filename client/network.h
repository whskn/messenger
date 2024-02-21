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

#define CONNECTION_TIMEOUT 5000
#define HANDSHAKE_SUCCESS "HS_SUCC"
#define HANDSHAKE_FAIL "HS_FAIL"

typedef char username_t[64];

typedef short msg_size_t;

extern int tryConnect(const char* ip, const int port, int* fd_ptr);

extern int sendMessage(int fd, const char* message, msg_size_t size);

extern int auth(int fd, username_t* username);
