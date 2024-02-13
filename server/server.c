#include "server.h"

int harvestConnection(const int port, conn_t* connection) {
    struct sockaddr_in address;
    int sockFd, connFd;
    
    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // TEHDOLG: error handling
        printf("error opening socket...\n"); 
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if ((bind(sockFd, (struct sockaddr*)&address, sizeof(address))) != 0) { 
        // TEHDOLG: error handling
        printf("bind failed...\n"); 
        return -1;
    } 

    if ((listen(sockFd, 5)) != 0) { 
        // TEHDOLG: error handling
        printf("Listen failed...\n"); 
        return -1;
    } 
   
    if (connFd = 
        (accept(sockFd, (struct sockaddr*)&address, sizeof(address)) < 0)) {
        // TEHDOLG: error handling
        printf("accepting failed...\n"); 
        return -1;
    }

    connection->sockFd = sockFd;
    connection->connFd = connFd;

    return 0;
}

int closeConnection(int sockFd) {
    if (close(sockFd) != 0) {
        // TEHDOLG: error handling
        printf("closing failed...\n"); 
        return -1;
    }

    return 0;
}

int sendMesage(int fd, const char* message, msg_size size) {
    if (send(fd, message, size, 0) != 0) {
        // TEHDOLG: error handling
        return -1;
    }

    return 0;
}

int connectionMainteiner() {
    
    // write new messages into message history
    // get new messages from message history
}