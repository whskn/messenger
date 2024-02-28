#include "network.h"
#include "../flags.h"

int tryConnect(const char* ip, const int port, int* fd_ptr) {
    struct sockaddr_in address;
    int fd;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // TEHDOLG: error handling
        return -1;
    }

    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    
    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        // TEHDOLG: error handling
        close(fd);
        return -1;
    } 

    *fd_ptr = fd;

    return 0;
}


int sendMessage(int fd, username_t to, const char* message, msg_size_t size) {
    size_t msgSize = size + sizeof(username_t);
    char* buffer = (char*)calloc(1, msgSize);
    memcpy(buffer, to, sizeof(username_t));
    memcpy(buffer + sizeof(username_t), message, size);

    if (write(fd, buffer, msgSize) < 0) {
        // TEHDOLG: error handling
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

int readMsg(int fd, username_t name, char* buffer, size_t size) {
    size_t bufferSize = sizeof(username_t) + size;
    char* tempBuffer = (char*)malloc(bufferSize);
    int ret = read(fd, tempBuffer, bufferSize);

    if (ret < 0) {
        printf("Problem with readMsg...\n");
        free(tempBuffer);
        return -1;
    } else if (ret == 0) {
        printf("EOF found while reading...\n");
        free(tempBuffer);
        return -1;
    }

    memcpy(name, tempBuffer, sizeof(username_t));
    memcpy(buffer, tempBuffer + sizeof(username_t), size);

    free(tempBuffer);
    return 0;
}

int auth(int fd, username_t username) {
    if (write(fd, username, sizeof(username_t)) < 0) {
        // TEHDOLG: error handling
        return -1;
    }

    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    int pollRet = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT * 1000);

    if (pollRet < 0) {
        // TEHDOLG: error handling
        printf("polling failed...\n"); 
        return -1;
    } else if (pollRet == 0) {
        // TEHDOLG: error handling
        printf("polling timed out...\n"); 
        return -1;
    }

    char code[4];
    if (read(fd, &code, sizeof(hs_code_t)) < 0) {
        // TEHDOLG: error handling
        printf("fail reading username while handshake...\n"); 
        return -1;
    } 
    
    if (*(int*)code == *(int*)HS_SUCC) {
        return 0;
    } else if (*(int*)code == *(int*)HS_MAX_CONN) {
        return -1;
    } else if (*(int*)code == *(int*)HS_INVAL_NAME) {
        return -1;
    } else if (*(int*)code == *(int*)HS_USER_EXISTS) {
        return -1;
    } 

    return 0;
}