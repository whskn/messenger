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

int closeConnection(conn_t* conn) {
    if (close(conn->sockFd) != 0) {
        // TEHDOLG: error handling
        printf("closing failed...\n"); 
        return -1;
    }

    return 0;
}

int sendMessage(int fd, const char* message, msg_size_t size) {
    if (send(fd, message, size, 0) != 0) {
        // TEHDOLG: error handling
        return -1;
    }

    return 0;
}

int manageConnection(conn_t conn, username_t* usernames, conn_t* connections) {
    username_t toUser;
    char msgBuffer[MAX_MESSAGE_LENGTH];
    struct pollfd fds;
    int pollRet;

    for (;;) {
        fds.fd = conn.connFd; 
        fds.events = POLLIN; 
        fds.revents = 0;

        // Blocking until message comes
        if ((pollRet = poll(&fds, (nfds_t)0, CONNECTION_TIMEOUT)) < 0) {
            // TEHDOLG: error handling
            printf("polling failed...\n"); 
            return -1;
        }

        // If no data available to read
        if (pollRet == 0) {
            closeConnection(conn.sockFd);
            printf("connection closed due to inactivity\n"); 
            break;
        }

        // Read the message
        if (read(conn.connFd, msgBuffer, sizeof(msgBuffer)) != 0) {
            // TEHDOLG: error handling
            printf("reading failed...\n"); 
            return -1;
        }

        memcpy(toUser, msgBuffer, size(toUser));
        
        int recipientId = -1;
        //lock semaphore
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (usernames[i] == NULL) continue;
            if (memcmp(usernames[i], toUser, sizeof(username_t))) {
                recipientId = i;
                break;
            }
        }
        //unlock semaphore

        if (recipientId > -1) {
            send(connections[recipientId].connFd, msgBuffer, MAX_MESSAGE_LENGTH, 0);
        }
        // write them in buffer if user is not online
    }

    return 0;
}