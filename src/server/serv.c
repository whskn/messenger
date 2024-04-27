#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include "serv.h"

#include "logger.h"
#include "../misc/validate.h"
#include "config.h"

#define EMPTY_FD (int)-2


#define HANDLE_SEND_ERROR(ret) \
    if (ret == NET_CHECK_ERRNO) { \
        logger(LOG_ERROR, "Failed to send message to user", false); \
        break; \
    } \
    if (ret == NET_CONN_BROKE) { \
        logger(LOG_INFO, "Connection broke", false); \
        break; \
    }


// TEHDOLG
int serv_init(conn_t*** conns, sem_t** mutex, const int port) {
    // allocating mem for connections

    conn_t** _conns = (conn_t**)calloc(MAX_CONNECTIONS, sizeof(conn_t*));
    if (_conns == NULL) {
        logger(LOG_ERROR, "Problem with calloc", true);
        return NET_CRITICAL_FAIL;
    }

    // getting a mutex
    sem_t* _mutex = (sem_t*)calloc(1, sizeof(sem_t));
    if (_mutex == NULL) {
        free(_conns);
        logger(LOG_ERROR, "Problem with calloc", true);
        return NET_CRITICAL_FAIL;
    }
    if (sem_init(_mutex, 0, 1) < 0) {
        free(_conns);
        free(_mutex);
        logger(LOG_ERROR, "Problem with sem_init", true);
        return NET_CRITICAL_FAIL;
    }

    int fd = net_open_sock(port);
    if (fd < 0) {
        free(_conns);
        free(_mutex);
        logger(LOG_ERROR, "Problem opening socket", true);
        return NET_CRITICAL_FAIL;
    }

    *conns = _conns;
    *mutex = _mutex;

    return fd;
}

int serv_close(int fd, conn_t** conns, sem_t** mutex) {
    sem_destroy(*mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (conns[i]) free(conns[i]);
    }
    free(*conns);
    free(*mutex);
    close(fd);
    // close all fd's
    
    *mutex = NULL;
    *conns = NULL;

    return NET_SUCCESS;
}



// -1 - no room, -2 - user online
int put_in_table(conn_t** conns, conn_t* my_conn) {
    int id = -1;

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (conns[i]) {
            if (conns[i]->user_id == my_conn->user_id){
                if (id >= 0) conns[id] = NULL;
                return -2;
            }
        } else if (id < 0) {
            conns[i] = my_conn;
            id = i;
        }
    }

    return id;
}

int auth_resp(const int fd, const int hs_code, const int user_id) {
    auth_res_t resp = {.cc = CC_AUTH, .hs_code = hs_code, .user_id = user_id};

    switch (net_send(fd, &resp, sizeof(auth_res_t)))
    {
    case NET_CHECK_ERRNO:
        logger(LOG_WARNING, "Error sending auth response: ", true);
        return -1;
    case NET_CONN_BROKE:
        logger(LOG_WARNING, "Error sending auth response, conn broke", false);
        return -1;
    }

    return 0; 
}

// -1 on fail
int blocking_read(conn_t* my_conn) {
    int ret;
    struct pollfd fds = {.fd = my_conn->fd, .events = POLLIN, .revents = 0};
    
    ret = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT);
    if (ret < 0) {
        logger(LOG_ERROR, "Error while polling casually", true);
        return -1;
    }
    if (ret == 0) {
        logger(LOG_INFO, "Connection closed due to inactivity", false);
        return -1;
    }

    // reading username
    ret = net_read(my_conn->fd, my_conn->buffer, MAX_PACKET_SIZE);
    if (ret == NET_ERROR) {
        logger(LOG_ERROR, "Error while polling casually", false);
        return -1;
    } 
    if (ret == NET_CHECK_ERRNO) {
        logger(LOG_ERROR, "Error while polling casually", true);
        return -1;
    }
    if (ret == NET_CONN_BROKE) {
        logger(LOG_INFO, "User disconnected", false);
        return -1;
    }

    return ret;
}

int auth_user(conn_t* my_conn, conn_t** conns) {
    auth_req_t* auth_msg = my_conn->buffer;
    user_t user;
    int ret;
    int id;

    if (!name_is_valid(auth_msg->username) ||
        !passwd_is_valid(auth_msg->password) ||
        !(auth_msg->auth_type | AUTH_LOGIN | AUTH_REGISTER)) {
            auth_resp(my_conn->fd, HS_GENERIC_ERROR, 0);
            return NET_AUTH_FAIL;
        }

    if (auth_msg->auth_type == AUTH_REGISTER) {
        ret = db_new_user(my_conn->db, auth_msg->username, auth_msg->password);
        if (ret == HST_CONSTRAINT) {
            auth_resp(my_conn->fd, HS_USER_EXISTS, 0);
            return NET_AUTH_FAIL;
        }
        if (ret < 0) {
            auth_resp(my_conn->fd, HS_GENERIC_ERROR, 0);
            return NET_AUTH_FAIL;
        }
    }

    // getting user
    ret = db_get_user(my_conn->db, auth_msg->username, &user);
    if (ret == HST_NOUSER) {
        auth_resp(my_conn->fd, HS_NO_USER, 0);
        return NET_AUTH_FAIL;
    }
    if (ret == HST_ERROR) {
        auth_resp(my_conn->fd, HST_ERROR, 0);
        return NET_AUTH_FAIL;
    }
    my_conn->user_id = user.user_id;

    // TEHDOLG password check

    ret = put_in_table(conns, my_conn);
    if (ret == -1) {
        auth_resp(my_conn->fd, HS_MAX_CONN, 0);
        return NET_AUTH_FAIL;
    }
    if (ret == -2) {
        auth_resp(my_conn->fd, HS_USER_EXISTS, 0);
        return NET_AUTH_FAIL;
    }
    id = ret;

    if (auth_resp(my_conn->fd, HS_SUCC, user.user_id) < 0) {
        conns[id] = NULL;
        return NET_AUTH_FAIL;
    }

    return id;
}

int auth_handler(conn_t* my_conn, conn_t** conns) {
    int read_bytes = blocking_read(my_conn);
    if (read_bytes != sizeof(auth_req_t)) {
        logger(LOG_ERROR, "Invalid auth request format", false);
        return NET_AUTH_FAIL;
    }

    return auth_user(my_conn, conns);
}


int serv_get_conn(int fd) {
    int user_fd;

    int ret = net_harvest_conn(fd);
    if (ret < 0) {
        return ret;
    }
    user_fd = ret;

    return user_fd;
}


int find_user(const int user_id, conn_t** conns, sem_t* mutex) {
    int fd = -1;

    sem_wait(mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!conns[i]) continue;
        if (user_id == conns[i]->user_id) {
            fd = conns[i]->fd;
            break;
        }
    }
    sem_post(mutex);

    return fd;
}


int flush_pending(conn_t* my_conn) {
    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));
    int ret;
    unsigned long timestamp = 0;

    while (true) {
        ret = db_next_unsent(my_conn->db, msg, my_conn->user_id, &timestamp);
        if (ret == HST_TABLE_EMPTY) {
            break;
        } 
        if (ret == HST_ERROR) {
            logger(LOG_WARNING, "Failed to pull message from database", false);
            break;
        }
        int message_id = ret;

        // checking msg for validity
        if (!msg_is_valid((void*)msg, msg_size(msg))) {
            logger(LOG_WARNING, "Invalid msg pulled from database", false);
            continue;
        }

        msg->cc = CC_MSG;
        ret = net_send(my_conn->fd, msg, msg_size(msg));
        HANDLE_SEND_ERROR(ret);

        ret = db_mark_as_sent(my_conn->db, message_id);
        if (ret != HST_SUCCESS) {
            logger(LOG_WARNING, "Failed to mark message as sent", false);
        }
    }

    free(msg);
    return NET_SUCCESS;
}


int msg_handler(conn_t* my_conn, conn_t** conns, sem_t* mutex, 
                const int readed_size) {
    int ret;
    msg_t* msg = my_conn->buffer;
    bool sent = false;

    // checking if recieved message is valid
    if (!msg_is_valid(msg, readed_size)) {
        logger(LOG_WARNING, "Invalid msg recieved", false);
        return 0;
    } 
    if (my_conn->user_id != msg->from_id) {
        logger(LOG_WARNING, "User's ids don't match", false);
        return 0;
    }

    // finding fd of the reciever
    int to_fd = find_user(msg->to_id, conns, mutex);

    // sending the message
    if (to_fd > -1) {
        // we suppose that readed_size is the right size of msg, since 
        // msg_is_valid() would detect if violated
        ret = net_send(to_fd, msg, msg_size(msg));
        if (ret == NET_SUCCESS) {
            sent = true;
        }
        else if (ret == NET_CHECK_ERRNO) { 
            logger(LOG_ERROR, "Failed to send message to user", false);
            return -1;
        }
    } 

    ret = db_push(my_conn->db, my_conn->buffer, sent);
    if (ret != HST_SUCCESS) {
        logger(LOG_ERROR, "Failed to push message to user's queue", false);
        return -1;
    }

    return 0;
}

int user_rqs_handler(conn_t* conn) {
    int ret;
    user_t user;
    user_req_t* req = conn->buffer; 
    user_rsp_t rsp = {.cc  = CC_USER_RQS, 
                      .exists = false, 
                      .user_id = -1};
    if (!name_is_valid(req->username)) {
        net_send(conn->fd, &rsp, sizeof(user_rsp_t));
        return 0;
    }

    ret = db_get_user(conn->db, req->username, &user);
    if (ret == HST_NOUSER) {
        net_send(conn->fd, &rsp, sizeof(user_rsp_t));
        return 0;
    }
    else if (ret != HST_SUCCESS) {
        logger(LOG_ERROR, "Failed to pull user from the database", false);
        net_send(conn->fd, &rsp, sizeof(user_rsp_t));
        return -1;
    }

    rsp.user_id = user.user_id;
    rsp.exists = true;
    memcpy(rsp.username, user.username, sizeof(username_t));

    ret = net_send(conn->fd, &rsp, sizeof(user_rsp_t));
    if (ret == NET_CHECK_ERRNO) {
        logger(LOG_ERROR, "Failed to send: ", true);
        return -1;
    }
    
    return 0;
}

int responder(conn_t* my_conn, conn_t** conns, sem_t* mutex) {
    int read_size = blocking_read(my_conn);
    if (read_size < (int)sizeof(int)) return -1;

    switch (*(int*)my_conn->buffer)
    {
    case CC_MSG:
        return msg_handler(my_conn, conns, mutex, read_size);

    case CC_USER_RQS:
        return user_rqs_handler(my_conn);

    default:
        logger(LOG_WARNING, "Message with unknown CC", false);
    }

    return 0;
}

int close_conn(conn_t* conn, sem_t* mutex) {
    if (mutex) sem_wait(mutex);

    if (net_close_conn(conn->fd) != NET_SUCCESS) {
        logger(LOG_ERROR, "Error closing socker of user connection", true);
    }

    if (db_close(conn->db) != HST_SUCCESS) {
        logger(LOG_ERROR, "Error while closing db", true);
    }

    if (conn->buffer) free(conn->buffer);
    free(conn);

    if (mutex) sem_post(mutex);
    return 0;
}

int remove_conn(const int conn_id, conn_t** conns, sem_t* mutex) {
    sem_wait(mutex);
    close_conn(conns[conn_id], NULL);
    conns[conn_id] = NULL;
    sem_post(mutex);

    return 0;
}

void* serv_manage_conn(void* void_args) {
    conn_t* my_conn = (conn_t*)calloc(sizeof(conn_t), 1);
    int my_conn_id;

    // retreiving arguments
    conn_t** conns = ((MC_arg_t*)void_args)->conns;
    sem_t* mutex   = ((MC_arg_t*)void_args)->mutex;
    my_conn->fd    = ((MC_arg_t*)void_args)->fd;

    int ret;

    ret = db_open(NULL, DB_FILENAME, &my_conn->db);
    if (ret != HST_SUCCESS) {
        logger(LOG_ERROR, "Cannot open db", false);
        close_conn(my_conn, mutex);
        return NULL;
    }

    my_conn->buffer = (void*)malloc(sizeof(msg_t));

    my_conn_id = auth_handler(my_conn, conns);
    if (my_conn_id < 0) {
        close_conn(my_conn, mutex);

        return NULL;
    }

    // flush all pending messages to the connected user
    flush_pending(my_conn);

    while(true) {
        if (responder(my_conn, conns, mutex) < 0) break;
    }

    remove_conn(my_conn_id, conns, mutex);
    return NULL;
}

// TEHDOLG set up proper mutual exclusion
