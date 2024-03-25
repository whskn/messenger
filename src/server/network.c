#include <stdio.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "../misc/blocking_read.h"
#include "network.h"
#include "logger.h"
#include <errno.h>

/**
 * This function opens main socket that will recieve connections.
 * 
 * @param port what port place this socket on
 * @param fd pointer to file descriptor
 * 
 * @return erorr codes
*/
int openMainSocket(const int port, int* fd) {
    struct sockaddr_in address;
    int tempFd;

    if ((tempFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return NET_CHECK_ERRNO;
    }

    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if ((bind(tempFd, (struct sockaddr*)&address, sizeof(address))) != 0) { 
        close(tempFd);
        return NET_CHECK_ERRNO;
    } 

    if ((listen(tempFd, CONN_QUEUE)) != 0) { 
        close(tempFd);
        return NET_CHECK_ERRNO;
    } 

    *fd = tempFd;
    return NET_SUCCESS;
}

/**
 * Get first connection from queue.
 * 
 * @param sockFd file descriptor of the socket
 * @param fd pointer to the variable, new file descriptor will be stored in
 * 
 * @return fd or error codes
*/
int harvestConnection(const int sockFd) {
    struct sockaddr_in cli;
    socklen_t cliLen;
    int fd;

    fd = accept(sockFd, (struct sockaddr*)&cli, &cliLen);
    if (fd < 0) {
        return NET_CHECK_ERRNO;
    }

    return fd;
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

bool username_is_valid(username_t name) {
    int i = 0;
    for (; i < (int)sizeof(username_t) && name[i] != '\0'; i++) {
        if (!(name[i] >= 48 && name[i] <= 57) && // numbers
            !(name[i] >= 65 && name[i] <= 90) && // capital letters
            !(name[i] >= 97 && name[i] <= 122))  // letters
        { 
            return false;
        }
    }

    if (i > 0 && name[i] == '\0') {
        return true;
    }

    return false;
}

/**
 * Try to authenticate user of a new connection.
 * 
 * @param fd file descripter of the connection with the user
 * @param conns array of all connections
 * @param mutex mutex to lock before using conns array
 * 
 * @return idx in conns array or error codes 
*/
int authUser(int fd, conn_t* conns, sem_t* mutex) {
    username_t authBuffer;
    bzero(authBuffer, sizeof(authBuffer));
    char code[HS_CODE_SIZE];
    int ret;

    // waiting for username
    ret = blocking_read(fd, authBuffer, sizeof(authBuffer), CONNECTION_TIMEOUT);
    if (ret == BR_TIMEOUT) {
        return NET_AUTH_FAIL;
    }
    if (ret == BR_CHECK_ERRNO) {
        return NET_CHECK_ERRNO;
    }
    if (ret == BR_EOF) {
        return NET_AUTH_FAIL;
    }
    
    // checking if the username is valid
    if (!username_is_valid(authBuffer)) {
        strcpy(code, HS_INVAL_NAME);
        write(fd, code, HS_CODE_SIZE);
        return NET_AUTH_FAIL;
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
            write(fd, code, sizeof(code));
            sem_post(mutex);
            return NET_AUTH_FAIL;
        } else if (id < 0) {
            conns[i].fd = fd;
            strcpy(conns[i].name, authBuffer);
            id = i;
        }
    }
    sem_post(mutex);

    // if no free places
    if (id < 0) {
        strcpy(code, HS_MAX_CONN);
        write(fd, code, sizeof(code));
        return NET_AUTH_FAIL;
    }

    strcpy(code, HS_SUCC);
    ret = write(fd, code, sizeof(code));
    if (ret < 0) {
        return NET_CHECK_ERRNO;
    } 
    if (ret == 0) {
        return NET_AUTH_FAIL;
    }

    return id;
}

/**
 * Close connection with a user
 * 
 * @param conn pointer to the exact connection to be closed in conn's array 
 * @param mutex mutex to wait for
 * 
 * @return error codes
*/
int closeConnection(conn_t* conn, sem_t* mutex) {
    if (close(conn->fd) != 0) {
        return NET_CHECK_ERRNO;
    }

    sem_wait(mutex);
    conn->fd = EMPTY_FD;
    sem_post(mutex);

    return NET_SUCCESS;
}


/**
 * Send message to a user. Message must be checked with msg_is_valid()
 * before calling this function.
 * 
 * @param fd file descriptor of the connection with the user
 * @param msg message itself in msg_t format
 * 
 * @return error codes
*/
int sendMessage(int fd, msg_t* msg) {
    int packet_size = msg->text_size + sizeof(*msg) - sizeof(msg->buffer);
    int ret;

    ret = write(fd, msg, packet_size);
    if (ret < 0) {
        return NET_CHECK_ERRNO;
    } 
    if (ret == 0) {
        return NET_CONN_BROKE;
    }

    return NET_SUCCESS;
}
