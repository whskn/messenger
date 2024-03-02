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

int clientConnect(const char* ip, const int port, username_t username) {
    int fd;

    if (tryConnect(ip, port, &fd) != 0) {
        printf("Failed to connect...\n");
        return -1;
    }

    if (auth(fd, username) < 0) {
        // TEHDOLG: error handling
        printf("auth failed...\n");
        return -2;
    }

    return fd;
}



int sendMessage(int fd, msg_t* msg) {
    ssize_t packet_size = msg->msg_size + sizeof(*msg) - sizeof(msg->buffer);
    if (packet_size < MIN_MESSAGE_LEN) {
        printf("Invalid message format...\n");
        return -2;
    }
    
    if (write(fd, msg, packet_size) < 0) {
        // TEHDOLG: error handling
        return -1;
    }

    return 0;
}

int readMsg(int fd, msg_t* msg, username_t me) {
    ssize_t ret = read(fd, msg, sizeof(*msg));

    if (ret < 0) {
        printf("Problem with readMsg...\n");
        return -1;
    } else if (ret == 0) {
        printf("EOF found while reading...\n");
        return -2;
    } 

    if (ret < MIN_MESSAGE_LEN || 
        (size_t)ret != (msg->msg_size + sizeof(*msg) - sizeof(msg->buffer)) ||
        msg->msg_size < 1 ||
        *(msg->names.to) == '\0' ||
        *(msg->names.from) == '\0' ||
        msg->timestamp == 0 ||
        memcmp(msg->names.to, me, sizeof(username_t)) != 0) {
        printf("Invalid message format recieved...\n");
        return -3;
    }

    return 0;
}
