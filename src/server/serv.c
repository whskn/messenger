#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "serv.h"

#include "logger.h"
#include "../history/history.h"
#include "../misc/blocking_read.h"
#include "../misc/validate.h"


#define DB_DIR "server_chats/"

#define HANDLE_SEND_ERROR(ret) \
    if (ret == NET_CHECK_ERRNO) { \
        logger(LOG_ERROR, "Failed to send message to user", false); \
        break; \
    } \
    if (ret == NET_CONN_BROKE) { \
        logger(LOG_INFO, "Connection broke", false); \
        break; \
    }

/**
 * Blocks current thread until new connection comes. Than auths new user
 * and returns id of the new connection. 
 * 
 * @param fd file descriptor of the main connection
 * @param conns connections array
 * @param mutex to lock before interactions with conns array
 * 
 * @return id or error code
*/
int get_conn(int fd, conn_t* conns, sem_t* mutex) {
    int connFd;

    int ret = harvestConnection(fd);
    if (ret < 0) {
        return ret;
    }
    connFd = ret;

    return authUser(connFd, conns, mutex);
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
        if (!strncmp(conns[i].name, name, sizeof(username_t))) {
            fd = conns[i].fd;
            break;
        }
    }
    sem_post(mutex);
    return fd;
}



/**
 * Send all pending messages to user
 * 
 * @param username username
 * @param fd connection's fd
 * 
 * @return error codes
*/
int flush_pending(char* username, int fd) {
    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));
    int ret;

    // LOCK DATABASE
    while (true) {
        ret = history_pull(DB_DIR, username, (void*)msg, sizeof(msg_t));
        if (ret == HST_TABLE_EMPTY) {
            break;
        } 
        if (ret == HST_ERROR) {
            logger(LOG_WARNING, "Failed to pull message from database", false);
            break;
        }
        int msg_size = ret;

        // checking msg for validity
        if (!msg_is_valid((void*)msg, msg_size)) {
            logger(LOG_WARNING, "Invalid msg pulled from database", false);
            continue;
        } 

        ret = sendMessage(fd, msg);
        HANDLE_SEND_ERROR(ret);
    }
    // UNLOCK DATABASE

    free(msg);
    return NET_SUCCESS;
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
    int ret;

    // flush all pending messages to the connected user
    flush_pending(conns[id].name, conns[id].fd);

    for (;;) {
        // Blocking until message comes
        ret = blocking_read(conns[id].fd, msg, sizeof(msg_t), CONNECTION_TIMEOUT);
        if (ret == BR_TIMEOUT) {
            logger(LOG_INFO, "Connection closed due to inactivity", false);
            break;
        }
        if (ret == BR_EOF) {
            logger(LOG_INFO, "EOF, connection is down", false);
            break;
        }
        if (ret == BR_CHECK_ERRNO) {
            logger(LOG_ERROR, "Error while polling casually", true);
            break;
        }
        int msg_size = ret;

        // checking if recieved message is valid
        if (!msg_is_valid((void*)msg, msg_size)) {
            logger(LOG_WARNING, "Invalid msg recieved", false);
            continue;
        } 

        // finding fd of the reciever
        int toFd = findUser(conns, msg->names.to, mutex);

        // sending the message
        if (toFd > -1) {
            ret = sendMessage(toFd, msg);
            HANDLE_SEND_ERROR(ret);
        } 
        else {
            const int size = msg->text_size - 
                             sizeof(msg->buffer) + 
                             sizeof(*msg);
            ret = history_push(DB_DIR, msg->names.to, (void*)msg, size);
            if (ret != HST_SUCCESS) {
                logger(LOG_ERROR, "Failed to push message to user's queue", 
                       false);
            }
        }
    }

    ret = closeConnection(&conns[id], mutex);
    if (ret != NET_SUCCESS) {
        logger(LOG_ERROR, "Error while closing connection", true);
    }

    free(msg);
    return NULL;
}