#include "network.h"
#include "../flags.h"

int openMainSocket(const int port) {
    struct sockaddr_in address;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // TEHDOLG: error handling
        printf("error opening socket...\n"); 
        exit(1);
    }

    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if ((bind(fd, (struct sockaddr*)&address, sizeof(address))) != 0) { 
        // TEHDOLG: error handling
        printf("bind failed...\n"); 
        close(fd);
        exit(1);
    } 

    if ((listen(fd, CONN_QUEUE)) != 0) { 
        // TEHDOLG: error handling
        printf("Listen failed...\n"); 
        exit(1);
    } 

    return fd;
}

int harvestConnection(const int sockFd) {
    struct sockaddr_in cli;
    socklen_t cliLen;
    int fd;

    if ((fd = accept(sockFd, (struct sockaddr*)&cli, &cliLen)) < 0) {
        // TEHDOLG: error handling
        printf("accepting failed...\n"); 
        return -1;
    }

    return fd;
}

int authUser(int fd, conn_t* conns, sem_t* mutex) {
    char msgBuffer[sizeof(username_t) + 1];

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

    printf("got auth message...\n");

    bzero(msgBuffer, sizeof(msgBuffer));
    if (read(fd, msgBuffer, sizeof(username_t)) < 1) {
        // TEHDOLG: error handling
        printf("fail reading username while handshake...\n"); 
        return -1;
    }

    printf("read auth msg...\n");

    if (msgBuffer[sizeof(msgBuffer) - 1]) {
        // TEHDOLG: error handling
        printf("username length must be <64 chars...\n");
        return -1;
    }
    
    int id = -1;
    sem_wait(mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        // need to check username if fd is not empty
        if (conns[i].fd != EMPTY_FD) {
            // usernames don't match -> great, iterate further
            if (memcmp(conns[i].name, msgBuffer, sizeof(username_t))) continue;

            // otherwise we have a problem, Huston
            printf("username exists...\n");
            if (id >= 0) conns[id].fd = EMPTY_FD;
            sendMessage(fd, HS_USER_EXISTS, sizeof(hs_code_t));
            return -1;
        }
        
        if (id < 0) {
            conns[i].fd = fd;
            memcpy(conns[i].name, msgBuffer, sizeof(username_t));
            id = i;
        }
    }
    sem_post(mutex);

    if (id < 0) {
        // TEHDOLG: error handling
        printf("server is full...\n"); 
        return -1;
    }

    sendMessage(fd, HS_SUCC, sizeof(hs_code_t));

    return id;
}

int closeConnection(conn_t* conn, sem_t* mutex) {
    if (close(conn->fd) != 0) {
        // TEHDOLG: error handling
        printf("closing failed...\n"); 
        return -1;
    }
    sem_wait(mutex);
    conn->fd = EMPTY_FD;
    sem_post(mutex);

    return 0;
}

int sendMessage(int fd, const char* message, msg_size_t size) {
    if (write(fd, message, size) < 0) {
        // TEHDOLG: error handling
        printf("error sending message...\n");
        return -1;
    }

    return 0;
}

void* manageConnection(void* void_args) {
    MC_arg_t* args = (MC_arg_t*)void_args;

    int id = args->id;
    conn_t* conns = args->conns;
    sem_t* mutex = args->mutex;

    char msgBuffer[MAX_MESSAGE_LENGTH];
    struct pollfd fds;
    int pollRet;

    for (;;) {
        fds.fd = conns[id].fd; 
        fds.events = POLLIN; 
        fds.revents = 0;

        // Blocking until message comes
        if ((pollRet = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT)) < 0) {
            // TEHDOLG: error handling
            printf("polling failed...\n"); 
            return NULL;
        }

        // If no data available to read
        if (pollRet == 0) {
            closeConnection(&conns[id], mutex);
            printf("connection closed due to inactivity\n"); 
            break;
        }

        // Read the message
        int ret = read(conns[id].fd, msgBuffer, sizeof(msgBuffer));
        if (ret < 0) {
            // TEHDOLG: error handling
            closeConnection(&conns[id], mutex);
            printf("reading failed...\n"); 
            return NULL;
        } else if (ret == 0) {
            closeConnection(&conns[id], mutex);
            printf("EOF conn closed...\n"); 
            return NULL;
        }

        int toFd = -1;

        sem_wait(mutex);
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (conns[i].fd == EMPTY_FD) continue;
            if (memcmp(conns[i].name, msgBuffer, sizeof(username_t)) == 0) {
                toFd = conns[i].fd;
                break;
            }
        }
        sem_post(mutex);

        if (toFd > -1) {
            memcpy(msgBuffer, conns[id].name, sizeof(username_t));
            sendMessage(toFd, msgBuffer, MAX_MESSAGE_LENGTH);
        }
        // write them in buffer if user is not online
    }

    return NULL;
}