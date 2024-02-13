#include "network.h"

int tryConnect(const char* ip, const int port, const int* fd_ptr) {
    struct sockaddr_in address;
    int status, fd;
    
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

    return 0;
}

int sendMesage(int fd, const char* message, msg_size size) {
    if (send(fd, message, size, 0)) {
        // TEHDOLG: error handling
        return -1;
    }

    return 0;
}