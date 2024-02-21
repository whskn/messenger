#include "network.h"


int harvestConnection(const int port, 
                      conn_t* connection, 
                      conn_t* connections, 
                      username_t* names) {
    struct sockaddr_in address;
    conn_t conn;
    
    if ((conn.sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // TEHDOLG: error handling
        printf("error opening socket...\n"); 
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if ((bind(conn.sockFd, (struct sockaddr*)&address, sizeof(address))) != 0) { 
        // TEHDOLG: error handling
        printf("bind failed...\n"); 
        return -1;
    } 

    if ((listen(conn.sockFd, 5)) != 0) { 
        // TEHDOLG: error handling
        printf("Listen failed...\n"); 
        return -1;
    } 
   
    if (conn.connFd = 
        (accept(conn.sockFd, (struct sockaddr*)&address, (socklen_t)sizeof(address)) < 0)) {
        // TEHDOLG: error handling
        printf("accepting failed...\n"); 
        return -1;
    }

    username_t name;
    if (!userIdent(conn, name)) {
        // TEHDOLG: error handling
        printf("handshake failed...\n"); 
        return -1;
    }

    // lock semaphore
    bool written = false;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (names[i] != NULL) continue;

        memcpy(names[i], name, sizeof(name));
        connections[i] = conn;
    }
    // unlock semaphore

    if (!written) {
        // TEHDOLG: error handling
        printf("max num of connections...\n"); 
        return 1;
    }

    *connection = conn;

    return 0;
}

int userIdent(conn_t conn, username_t username) {
    const char* code = HANDSHAKE_CODE;
    const int hsCodeLen = sizeof(code) * 2 + sizeof(username_t);
    char* msgBuffer[hsCodeLen];

    memset(msgBuffer, 0, sizeof(msgBuffer));

    if (read(conn.sockFd, msgBuffer, sizeof(msgBuffer)) < 0) {
        // TEHDOLG: error handling
        printf("reading handshake failed...\n"); 
        return -1;
    }

    // comparing handshake codes
    const int endCodeIdx = hsCodeLen - sizeof(code);
    if (!memcmp(msgBuffer, code, sizeof(code)) || 
        !memcmp(msgBuffer[endCodeIdx], code, sizeof(code))) {
        printf("Handshake code is wrong...\n");
        return 1;
    }
    
    memcpy(username, msgBuffer[sizeof(code)], sizeof(username_t));
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

void* manageConnection(void* void_args) {
    MC_arg_t* args = (MC_arg_t*)void_args;
    username_t toUser;
    char msgBuffer[MAX_MESSAGE_LENGTH];
    struct pollfd fds;
    int pollRet;

    for (;;) {
        fds.fd = args->conn->connFd; 
        fds.events = POLLIN; 
        fds.revents = 0;

        // Blocking until message comes
        if ((pollRet = poll(&fds, (nfds_t)0, CONNECTION_TIMEOUT)) < 0) {
            // TEHDOLG: error handling
            printf("polling failed...\n"); 
            return NULL;
        }

        // If no data available to read
        if (pollRet == 0) {
            closeConnection(args->conn->sockFd);
            printf("connection closed due to inactivity\n"); 
            break;
        }

        // Read the message
        if (read(args->conn->connFd, msgBuffer, sizeof(msgBuffer)) != 0) {
            // TEHDOLG: error handling
            printf("reading failed...\n"); 
            return NULL;
        }

        memcpy(toUser, msgBuffer, sizeof(toUser));
        
        int recipientId = -1;
        //lock semaphore
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (args->usernames[i] == NULL) continue;
            if (memcmp(args->usernames[i], toUser, sizeof(username_t))) {
                recipientId = i;
                break;
            }
        }
        //unlock semaphore

        if (recipientId > -1) {
            send(args->connections[recipientId].connFd, 
                 msgBuffer, 
                 MAX_MESSAGE_LENGTH, 
                 0);
        }
        // write them in buffer if user is not online
    }

    return NULL;
}