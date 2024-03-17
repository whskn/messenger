#include "network.h"
#include "logger.h"
#include <errno.h>

/**
 * This function opens main socket that will recieve connections.
 * 
 * @param port what port place this socket on
 * @param fd pointer to file descriptor
 * 
 * @return 0 - success; 
 *         1 - one of syscalls failed, check errno.
*/
int openMainSocket(const int port, int* fd) {
    struct sockaddr_in address;
    int tempFd;

    if ((tempFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return 1;
    }

    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if ((bind(tempFd, (struct sockaddr*)&address, sizeof(address))) != 0) { 
        close(tempFd);
        return 1;
    } 

    if ((listen(tempFd, CONN_QUEUE)) != 0) { 
        close(tempFd);
        return 1;
    } 

    *fd = tempFd;
    return 0;
}

/**
 * Get first connection from queue.
 * 
 * @param sockFd file descriptor of the socket
 * @param fd pointer to the variable, new file descriptor will be stored in
 * 
 * @return 0 - success;
 *         1 - syscall failed, check errno;
*/
int harvestConnection(const int sockFd, int* fd) {
    struct sockaddr_in cli;
    socklen_t cliLen;

    int tempFd;
    if ((tempFd = accept(sockFd, (struct sockaddr*)&cli, &cliLen)) < 0) {
        return 1;
    }

    *fd = tempFd;

    return 0;
}

/**
 * My strnlen implementation, since the one from string.h is somehow not 
 * avaliable.
*/
size_t strnlen(const char* s, size_t len) {
    size_t i = 0;
    for (; i < len && s[i] != '\0'; ++i);
    return i;
}

/**
 * Try to authenticate user of a new connection.
 * 
 * @param fd file descripter of the connection with the user
 * @param conns array of all connections
 * @param mutex mutex to lock before using conns array
 * 
 * @return 0 - success; 
 *         1 - syscall failed, check errno;
 *         2 - timed out;
 *         3 - auth unsuccessful;
*/
int authUser(int fd, int* idptr, conn_t* conns, sem_t* mutex) {
    username_t authBuffer;
    bzero(authBuffer, sizeof(authBuffer));
    char code[HS_CODE_SIZE];

    // waiting for username
    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    int pollRet = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT);
    if (pollRet < 0) return 1;
    else if (pollRet == 0) return 2;

    // reading username
    if (read(fd, authBuffer, sizeof(authBuffer)) < 1) return 1;
    
    // checking if last byte is 0 (if name length is appropirate)
    if (strnlen(authBuffer, sizeof(username_t)) >= 32) {
        strcpy(code, HS_INVAL_NAME);
        if (write(fd, code, HS_CODE_SIZE) != HS_CODE_SIZE) return 1;
        return 3;
    }
    
    // checking if such username is already present
    int id = -1;
    sem_wait(mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        // need to check username if fd is not empty
        if (conns[i].fd != EMPTY_FD) {
            // usernames don't match -> great, iterate further
            if (strncmp(conns[i].name, authBuffer, sizeof(authBuffer))) continue;

            // otherwise we have a problem, Huston
            if (id >= 0) conns[id].fd = EMPTY_FD;
            strcpy(code, HS_USER_EXISTS);
            if (write(fd, code, sizeof(code)) != sizeof(code)) {
                sem_post(mutex);
                return 1;
            } 
            sem_post(mutex);
            return 3;
        } else if (id < 0) {
            conns[i].fd = fd;
            strcpy(conns[i].name, authBuffer);
            id = i;
        }
    }
    sem_post(mutex);

    if (id < 0) {
        strcpy(code, HS_MAX_CONN);
        if (write(fd, code, sizeof(code)) != sizeof(code)) return 1;
        return 3;
    }

    strcpy(code, HS_SUCC);
    if (write(fd, code, sizeof(code)) != sizeof(code)) return 1;
    *idptr = id;

    return 0;
}

/**
 * Close connection with a user
 * 
 * @param conn pointer to the exact connection to be closed in conn's array 
 * @param mutex mutex to wait for
 * 
 * @return 0 - success;
 *         1 - syscall failed, check errno;
*/
int closeConnection(conn_t* conn, sem_t* mutex) {
    if (close(conn->fd) != 0) return 1;

    sem_wait(mutex);
    conn->fd = EMPTY_FD;
    sem_post(mutex);

    return 0;
}

/**
 * Send message to a user
 * 
 * @param fd file descriptor of the connection with the user
 * @param msg message itself in msg_t format
 * 
 * @return 0 - success; 1 - syscall error, check errno
*/
int sendMessage(int fd, msg_t* msg) {
    ssize_t packet_size = msg->msg_size + sizeof(*msg) - sizeof(msg->buffer);
    if (write(fd, msg, packet_size) < 0) return 1;
    return 0;
}
