#include "network.h"

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

int sendMessage(int fd, const char* message, msg_size_t size) {
    if (write(fd, message, size) < 0) {
        // TEHDOLG: error handling
        return -1;
    }

    return 0;
}

int auth(int fd, username_t* username) {
    int authBufferSize = CODE_SIZE + sizeof(username_t);
    char* authBuffer = (char*)calloc(1, authBufferSize);
    memcpy(authBuffer, HANDSHAKE_CODE, CODE_SIZE);
    memcpy(authBuffer + CODE_SIZE, *username, sizeof(username_t));

    if (sendMessage(fd, authBuffer, authBufferSize) < 0) {
        // TEHDOLG: error handling
        free(authBuffer);
        return -1;
    }
    free(authBuffer);

    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    int pollRet = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT);

    if (pollRet < 0) {
        // TEHDOLG: error handling
        printf("polling failed...\n"); 
        return -1;
    } else if (pollRet == 0) {
        // TEHDOLG: error handling
        printf("polling timed out...\n"); 
        return -1;
    }

    char buffer[sizeof(HANDSHAKE_FAIL) - 1];
    if (read(fd, buffer, sizeof(HANDSHAKE_FAIL) - 1) < 0) {
        // TEHDOLG: error handling
        printf("fail reading username while handshake...\n"); 
        return -1;
    } 
    
    if (memcmp(buffer, HANDSHAKE_SUCCESS, sizeof(buffer)) == 0) {
        printf("handshake successful...\n");
        return 0;
    }

    return -1;
}