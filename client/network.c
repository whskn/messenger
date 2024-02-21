#include "network.h"

int tryConnect(const char* ip, const int port, int* fd_ptr) {
    struct sockaddr_in address;
    int fd;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // TEHDOLG: error handling
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        // TEHDOLG: error handling
        return -1;
    } 

    *fd_ptr = fd;

    return 0;
}

int sendMessage(int fd, const char* message, msg_size_t size) {
    if (send(fd, message, size, 0)) {
        // TEHDOLG: error handling
        return -1;
    }

    return 0;
}