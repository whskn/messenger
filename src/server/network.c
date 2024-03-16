#include "network.h"
#include "../flags.h"
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
    char msgBuffer[sizeof(username_t)];
    bzero(msgBuffer, sizeof(msgBuffer));

    // waiting for username
    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    int pollRet = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT);
    if (pollRet < 0) return 1;
    else if (pollRet == 0) return 2;

    // reading username
    if (read(fd, msgBuffer, sizeof(username_t)) < 1) return 1;
    
    // checking if such username is already present
    int id = -1;
    sem_wait(mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        // need to check username if fd is not empty
        if (conns[i].fd != EMPTY_FD) {
            // usernames don't match -> great, iterate further
            if (memcmp(conns[i].name, msgBuffer, sizeof(username_t))) continue;

            // otherwise we have a problem, Huston
            if (id >= 0) conns[id].fd = EMPTY_FD;
            if (write(fd, HS_USER_EXISTS, sizeof(hs_code_t)) != 
                sizeof(hs_code_t)) {
                    sem_post(mutex);
                    return 1;
                } 
            sem_post(mutex);
            return 3;
        } else if (id < 0) {
            conns[i].fd = fd;
            memcpy(conns[i].name, msgBuffer, sizeof(username_t));
            id = i;
        }
    }
    sem_post(mutex);

    if (id < 0) {
        if (write(fd, HS_MAX_CONN, sizeof(hs_code_t)) != sizeof(hs_code_t)) {
            return 1;
        }
        return 3;
    }

    if (write(fd, HS_SUCC, sizeof(hs_code_t)) != sizeof(hs_code_t)) return 1;
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

/**
 * Finds file descriptor of user's connection through the username
 * 
 * @param conns array of connections
 * @param name username of the user 
 * @param mutex mutex to lock before search
 * 
 * @return file descriptor if found, -1 otherwise
*/
int findUser(conn_t* conns, username_t name, sem_t* mutex) {
    int fd = -1;
    sem_wait(mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (conns[i].fd == EMPTY_FD) continue;
        if (memcmp(conns[i].name, name, sizeof(username_t)) == 0) {
            fd = conns[i].fd;
            break;
        }
    }
    sem_post(mutex);
    return fd;
}

/**
 * Checks msg_t structure for validity
 * 
 * @param readRet return value of read() (needed in checks)
 * @param msg msg_t to check 
 * @return valid or not
*/
bool messageIsValid(size_t readRet, msg_t* msg) {
    if (readRet < MIN_MESSAGE_LEN || 
        (size_t)readRet != (msg->msg_size + sizeof(*msg) - sizeof(msg->buffer)) ||
        msg->msg_size < 1 ||
        msg->timestamp == 0 ||
        *(msg->names.to) == '\0' ||
        *(msg->names.from) == '\0') {
        return false;
    }
    return true;
}

/**
 * Function to call to serve incoming connections 
 * 
 * @param void_args pointer to MC_arg_t casted to void*  
*/
void* manageConnection(void* void_args) {
    MC_arg_t* args = (MC_arg_t*)void_args;

    // retreiving arguments
    int id = args->id;
    conn_t* conns = args->conns;
    sem_t* mutex = args->mutex;

    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));

    for (;;) {
        // Blocking until message comes
        struct pollfd fds = {.fd = conns[id].fd, .events = POLLIN, .revents = 0};
        if (poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT) <= 0) {
            printf("Poll failed, errno: %s\n", strerror(errno));
            break;
        }

        // Read the message
        int readRet = read(conns[id].fd, msg, sizeof(*msg));
        if (readRet < 0) {
            printf("Failed to read message, errno: %s\n", strerror(errno));
            break;
        } else if (readRet == 0) {
            printf("EOF, connection closed...\n");
            break;
        }

        // checking message for validity
        if (!messageIsValid(readRet, msg)) {
            printf("Recieved an invalid message...\n");
            continue;
        } 

        // finding fd of the reciever 
        int toFd = findUser(conns, msg->names.to, mutex);

        // sending the message
        if (toFd > -1) sendMessage(toFd, msg);
    }

    if (closeConnection(&conns[id], mutex) != 0) {
        printf("Failed to read message, errno: %s\n", strerror(errno));
    }
    free(msg);

    return NULL;
}
