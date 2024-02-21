#include "network.h"


int harvestConnection(const int port, 
                      conn_t* connection, 
                      conn_t* connections, 
                      username_t* names) {
    struct sockaddr_in address, cli;
    socklen_t cliLen;
    conn_t conn;
    
    if ((conn.sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // TEHDOLG: error handling
        printf("error opening socket...\n"); 
        return -1;
    }

    bzero(&address, sizeof(address)); 
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
   
    if ((conn.connFd = 
        accept(conn.sockFd, (struct sockaddr*)&cli, &cliLen)) < 0) {
        // TEHDOLG: error handling
        printf("accepting failed...\n"); 
        return -1;
    }

    username_t name;
    if (userIdent(conn, name) < 0) {
        // TEHDOLG: error handling
        close(conn.sockFd);
        printf("handshake failed...\n"); 
        return -1;
    }

    // lock semaphore
    bool written = false;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if ((long long int)names[i] != 0) continue;

        memcpy(names[i], name, sizeof(name));
        connections[i] = conn;
    }
    // unlock semaphore

    if (!written) {
        // TEHDOLG: error handling
        close(conn.sockFd);
        printf("max num of connections...\n"); 
        return 1;
    }

    *connection = conn;

    return 0;
}

int userIdent(conn_t conn, username_t username) {
    const char* code = HANDSHAKE_CODE;
    const int sizeOfCode = sizeof(code);
    const int hsCodeLen = sizeOfCode * 2 + sizeof(username_t);

    char* msgBuffer[hsCodeLen];

    memset(msgBuffer, 0, sizeof(msgBuffer));

    struct pollfd fds = {.fd = conn.connFd, .events = POLLIN, .revents = 0};
    int pollRet = poll(&fds, (nfds_t)0, CONNECTION_TIMEOUT);

    if (pollRet < 0) {
        // TEHDOLG: error handling
        printf("polling failed...\n"); 
        return -1;
    } else if (pollRet == 0) {
        // TEHDOLG: error handling
        printf("polling timed out...\n"); 
        return -1;
    }

    if (read(conn.connFd, msgBuffer, hsCodeLen) < 0) {
        // TEHDOLG: error handling
        printf("fail reading username while handshake...\n"); 
        return -1;
    }

    // TEHDOLG: comparing handshake codes

    const int endCodeIdx = hsCodeLen - sizeOfCode;
    if (!memcmp(msgBuffer, code, sizeOfCode) || 
        !memcmp(msgBuffer[endCodeIdx], code, sizeOfCode)) {
        printf("Handshake code is wrong...\n");
        return -1;
    }

    // check if name is already present
    const char* hsSuccess = HANDSHAKE_SUCCESS;
    if (write(conn.connFd, hsSuccess, sizeof(hsSuccess)) < 0 ) {
        // TEHDOLG: error handling
        printf("fail sending handshake success code...\n"); 
        return -1;
    }
    
    memcpy(username, msgBuffer[sizeOfCode], sizeof(username_t));
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
    if (write(fd, message, size) < 0) {
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
            closeConnection(args->conn);
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
            if ((long long int)args->usernames[i] == 0) continue;
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