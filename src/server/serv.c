#include "serv.h"
#include "logger.h"
#include "../history/history.h"

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
        if (!strncmp(conns[i].name, name, sizeof(username_t))) {
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
            logger(LOG_ERROR, "Poll failed", true);
            break;
        }

        // Read the message
        int readRet = read(conns[id].fd, msg, sizeof(*msg));
        if (readRet < 0) {
            logger(LOG_WARNING, "Failed to read message", true);
            break;
        } else if (readRet == 0) {
            logger(LOG_INFO, "EOF, connection closed", false);
            break;
        }

        // checking message for validity
        if (!messageIsValid(readRet, msg)) {
            logger(LOG_WARNING, "Invalid message format", false);
            continue;
        } 

        // finding fd of the reciever 
        int toFd = findUser(conns, msg->names.to, mutex);

        // sending the message
        if (toFd > -1) {
            sendMessage(toFd, msg);
        } else {
            // ...
        }

    }

    if (closeConnection(&conns[id], mutex) != 0) {
        logger(LOG_ERROR, "Error while closing connection", true);
    }
    free(msg);

    return NULL;
}