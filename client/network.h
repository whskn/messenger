#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>

typedef short msg_size;

extern int tryConnect(const char* ip, const int port, const int* fd_ptr);

extern int sendMesage(int fd, const char* message, msg_size size);
