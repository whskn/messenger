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

// VV
int serv_init(conn_t ***conns, mtx_t **page_mtx, const int port)
{
    // allocating mem for connections
    conn_t **_conns;
    mtx_t *_page_mtx;

    _conns = (conn_t **)calloc(MAX_CONNECTIONS, sizeof(conn_t *));
    _page_mtx = (mtx_t *)malloc(sizeof(mtx_t));

    // getting a page-mutex
    if (pthread_mutex_init(_page_mtx, NULL))
    {
        free(_conns);
        free(_page_mtx);
        logger(LOG_ERROR, "Failed to create a mutex", false);
        return NET_CRITICAL_FAIL;
    }

    int fd = net_open_sock(port);
    if (fd < 0)
    {
        free(_conns);
        free(_page_mtx);
        logger(LOG_ERROR, "Failed to open a socket", true);
        return NET_CRITICAL_FAIL;
    }

    *conns = _conns;
    *page_mtx = _page_mtx;

    return fd;
}

// VV
int serv_close(int fd, conn_t **conns, mtx_t *page_mtx)
{
    conn_t *conn;

    pthread_mutex_lock(page_mtx);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        conn = conns[i];
        conns[i] = NULL;

        if (conn)
        {
            if (pthread_cancel(conn->thread_id))
            {
                logger(LOG_ERROR, "Failed to cancel thread: ", true);
            }
            if (net_close_conn(conn->fd) != NET_SUCCESS)
            {
                logger(LOG_ERROR, "Error closing socket of user connection: ", true);
            }
            if (db_close(conn->db) != HST_SUCCESS)
            {
                logger(LOG_ERROR, "Error while closing db", true);
            }
            // do not destroy mutex, cause if it's locked, this op can lead to unexpectes consequences
            if (conn->buffer)
                free(conn->buffer);
        }
    }
    pthread_mutex_unlock(page_mtx);
    pthread_mutex_destroy(page_mtx);
    free(page_mtx);

    free(conns);

    net_close_conn(fd);

    return NET_SUCCESS;
}

// -1 - no room, -2 - user online
// VV
int put_in_table(conn_t **conns, conn_t *my_conn, mtx_t *page_mtx)
{
    int id = -1;

    pthread_mutex_lock(page_mtx);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (conns[i])
        {
            if (conns[i]->user_id == my_conn->user_id)
            {
                if (id >= 0)
                    conns[id] = NULL;
                pthread_mutex_unlock(page_mtx);
                return -2;
            }
        }
        else if (id < 0)
        {
            conns[i] = my_conn;
            id = i;
        }
    }
    pthread_mutex_unlock(page_mtx);

    return id;
}

int lock_n_send(conn_t *conn, void *buffer, const int size)
{
    int ret;

    pthread_mutex_lock(&conn->conn_mtx);
    ret = net_send(conn->fd, buffer, size);
    pthread_mutex_unlock(&conn->conn_mtx);

    return ret;
}

// --
int auth_resp(const int fd, const int hs_code, const int user_id)
{
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
// --
int blocking_read(conn_t *my_conn)
{
    int ret;
    struct pollfd fds = {.fd = my_conn->fd, .events = POLLIN, .revents = 0};

    ret = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT);
    if (ret < 0)
    {
        logger(LOG_ERROR, "Error while polling casually", true);
        return -1;
    }
    if (ret == 0)
    {
        logger(LOG_INFO, "Connection closed due to inactivity", false);
        return -1;
    }

    ret = net_read(my_conn->fd, my_conn->buffer, MAX_PACKET_SIZE);
    if (ret == NET_ERROR)
    {
        logger(LOG_ERROR, "Error while polling casually", false);
        return -1;
    }
    if (ret == NET_CHECK_ERRNO)
    {
        logger(LOG_ERROR, "Error while polling casually", true);
        return -1;
    }
    if (ret == NET_CONN_BROKE)
    {
        logger(LOG_INFO, "User disconnected", false);
        return -1;
    }

    return ret;
}

// --
int auth_user(conn_t *my_conn, conn_t **conns, mtx_t *page_mtx)
{
    auth_req_t *auth_msg = my_conn->buffer;
    user_t user;
    int ret;
    int id;

    if (!name_is_valid(auth_msg->username) ||
        !passwd_is_valid(auth_msg->password) ||
        !(auth_msg->auth_type | AUTH_LOGIN | AUTH_REGISTER))
    {
        auth_resp(my_conn->fd, HS_GENERIC_ERROR, 0);
        return NET_AUTH_FAIL;
    }

    if (auth_msg->auth_type == AUTH_REGISTER)
    {
        ret = db_new_user(my_conn->db, auth_msg->username, auth_msg->password);
        if (ret == HST_CONSTRAINT)
        {
            auth_resp(my_conn->fd, HS_USER_EXISTS, 0);
            return NET_AUTH_FAIL;
        }
        if (ret < 0)
        {
            auth_resp(my_conn->fd, HS_GENERIC_ERROR, 0);
            return NET_AUTH_FAIL;
        }
    }

    // getting user
    ret = db_get_user(my_conn->db, auth_msg->username, &user);
    if (ret == HST_NOUSER)
    {
        auth_resp(my_conn->fd, HS_NO_USER, 0);
        return NET_AUTH_FAIL;
    }
    if (ret == HST_ERROR)
    {
        auth_resp(my_conn->fd, HST_ERROR, 0);
        return NET_AUTH_FAIL;
    }
    my_conn->user_id = user.user_id;

    // TEHDOLG password check

    pthread_mutex_lock(&my_conn->conn_mtx);
    ret = put_in_table(conns, my_conn, page_mtx);
    if (ret == -1)
    {
        auth_resp(my_conn->fd, HS_MAX_CONN, 0);
        return NET_AUTH_FAIL;
    }
    if (ret == -2)
    {
        auth_resp(my_conn->fd, HS_USER_EXISTS, 0);
        return NET_AUTH_FAIL;
    }
    id = ret;

    if (auth_resp(my_conn->fd, HS_SUCC, user.user_id) < 0)
    {
        conns[id] = NULL;
        return NET_AUTH_FAIL;
    }
    pthread_mutex_unlock(&my_conn->conn_mtx);

    return id;
}

// VV
int auth_handler(conn_t *my_conn, conn_t **conns, mtx_t *page_mtx)
{
    int read_bytes = blocking_read(my_conn);
    if (read_bytes != sizeof(auth_req_t))
    {
        logger(LOG_ERROR, "Invalid auth request format", false);
        return NET_AUTH_FAIL;
    }

    return auth_user(my_conn, conns, page_mtx);
}

// --
int serv_get_conn(int fd)
{
    int user_fd;

    int ret = net_harvest_conn(fd);
    if (ret < 0)
    {
        return ret;
    }
    user_fd = ret;

    return user_fd;
}

// VV
conn_t *find_user(const int user_id, conn_t **conns, mtx_t *page_mtx)
{
    conn_t *conn_ptr = NULL;

    pthread_mutex_lock(page_mtx);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (conns[i] && user_id == conns[i]->user_id)
        {
            pthread_mutex_lock(&conns[i]->conn_mtx);
            conn_ptr = conns[i];
            break;
        }
    }
    pthread_mutex_unlock(page_mtx);

    return conn_ptr;
}

// --
int flush_pending(conn_t *my_conn)
{
    msg_t *msg = (msg_t *)calloc(1, sizeof(msg_t));
    int ret;
    unsigned long timestamp = 0;

    while (true)
    {
        ret = db_next_unsent(my_conn->db, msg, my_conn->user_id, &timestamp);
        if (ret == HST_TABLE_EMPTY)
        {
            break;
        }
        if (ret == HST_ERROR)
        {
            logger(LOG_WARNING, "Failed to pull message from database", false);
            break;
        }
        int message_id = ret;

        // checking msg for validity
        if (!msg_is_valid((void *)msg, msg_size(msg)))
        {
            logger(LOG_WARNING, "Invalid msg pulled from database", false);
            continue;
        }

        msg->cc = CC_MSG;
        ret = lock_n_send(my_conn, msg, msg_size(msg));
        if (ret == NET_CHECK_ERRNO)
        {
            logger(LOG_ERROR, "Failed to send message to user", false);
            break;
        }
        if (ret == NET_CONN_BROKE)
        {
            logger(LOG_INFO, "Connection broke", false);
            break;
        }

        ret = db_mark_as_sent(my_conn->db, message_id);
        if (ret != HST_SUCCESS)
        {
            logger(LOG_WARNING, "Failed to mark message as sent", false);
        }
    }

    free(msg);
    return NET_SUCCESS;
}

// VV
int msg_handler(conn_t *my_conn, conn_t **conns, mtx_t *page_mtx,
                const int readed_size)
{
    int ret;
    msg_t *msg = my_conn->buffer;
    bool sent = false;
    conn_t *reciever;

    // checking if recieved message is valid
    if (!msg_is_valid(msg, readed_size))
    {
        logger(LOG_WARNING, "Invalid msg recieved", false);
        return 0;
    }
    if (my_conn->user_id != msg->from_id)
    {
        logger(LOG_WARNING, "User's ids don't match", false);
        return 0;
    }

    // finding fd of the reciever
    reciever = find_user(msg->to_id, conns, page_mtx); // MUTEX DOWN

    // sending the message
    if (reciever)
    {
        ret = net_send(reciever->fd, msg, msg_size(msg)); // MUTEX UP
        pthread_mutex_unlock(&reciever->conn_mtx);
        if (ret == NET_SUCCESS)
        {
            sent = true;
        }
        else if (ret == NET_CHECK_ERRNO)
        {
            logger(LOG_ERROR, "Failed to send message to user", false);
            return -1;
        }
    }

    ret = db_push(my_conn->db, my_conn->buffer, sent);
    if (ret < 0)
    {
        logger(LOG_ERROR, "Failed to push message db", false);
        return -1;
    }

    return 0;
}

int user_rqs_handler(conn_t *conn)
{
    int ret;
    user_t user;
    user_req_t *req = conn->buffer;
    user_rsp_t rsp = {.cc = CC_USER_RQS,
                      .exists = false,
                      .user_id = -1};
    if (!name_is_valid(req->username))
    {
        lock_n_send(conn, &rsp, sizeof(user_rsp_t));
        return 0;
    }

    ret = db_get_user(conn->db, req->username, &user);
    if (ret == HST_NOUSER)
    {
        lock_n_send(conn, &rsp, sizeof(user_rsp_t));
        return 0;
    }
    else if (ret != HST_SUCCESS)
    {
        logger(LOG_ERROR, "Failed to pull user from the database", false);
        lock_n_send(conn, &rsp, sizeof(user_rsp_t));
        return -1;
    }

    rsp.user_id = user.user_id;
    rsp.exists = true;
    memcpy(rsp.username, user.username, sizeof(username_t));

    ret = lock_n_send(conn, &rsp, sizeof(user_rsp_t));
    if (ret == NET_CHECK_ERRNO)
    {
        logger(LOG_ERROR, "Failed to send: ", true);
        return -1;
    }

    return 0;
}

// --
int responder(conn_t *my_conn, conn_t **conns, mtx_t *page_mutex)
{
    int read_size = blocking_read(my_conn);
    if (read_size < (int)sizeof(int))
        return -1;

    switch (*(int *)my_conn->buffer)
    {
    case CC_MSG:
        return msg_handler(my_conn, conns, page_mutex, read_size);

    case CC_USER_RQS:
        return user_rqs_handler(my_conn);

    default:
        logger(LOG_WARNING, "Message with unknown CC", false);
    }

    return 0;
}

// use only if conn is not in the page
// VV
int close_conn(conn_t *conn)
{
    if (!pthread_mutex_trylock(&conn->conn_mtx))
    {
        pthread_mutex_unlock(&conn->conn_mtx);
        pthread_mutex_destroy(&conn->conn_mtx);
    }
    else
    {
        logger(LOG_ERROR, "Mutex of the connection is locked: ", true);
    }

    if (net_close_conn(conn->fd) != NET_SUCCESS)
    {
        logger(LOG_ERROR, "Error closing socket of user connection: ", true);
    }
    if (db_close(conn->db) != HST_SUCCESS)
    {
        logger(LOG_ERROR, "Error while closing db", true);
    }

    if (conn->buffer)
        free(conn->buffer);

    free(conn);
    return 0;
}

// only callable by thread whose connection is to be deleted
// VV
int remove_conn(const int conn_id, conn_t **conns, mtx_t *page_mtx)
{
    conn_t *to_close = conns[conn_id];

    pthread_mutex_lock(page_mtx);
    conns[conn_id] = NULL;
    pthread_mutex_unlock(page_mtx);

    close_conn(to_close);

    return 0;
}

void *serv_manage_conn(void *void_args)
{
    conn_t *my_conn = (conn_t *)calloc(sizeof(conn_t), 1);
    int my_conn_id;

    // retreiving arguments
    conn_t **conns = ((MC_arg_t *)void_args)->conns;
    mtx_t *page_mtx = ((MC_arg_t *)void_args)->page_mtx;
    my_conn->fd = ((MC_arg_t *)void_args)->fd;
    free(void_args);
    pthread_mutex_init(&my_conn->conn_mtx, NULL);

    int ret;

    ret = db_open(NULL, DB_FILENAME, &my_conn->db);
    if (ret != HST_SUCCESS)
    {
        logger(LOG_ERROR, "Cannot open db", false);
        close_conn(my_conn);
        return NULL;
    }

    my_conn->buffer = (void *)malloc(sizeof(msg_t));

    my_conn_id = auth_handler(my_conn, conns, page_mtx);
    if (my_conn_id < 0)
    {
        close_conn(my_conn);

        return NULL;
    }

    // flush all pending messages to the connected user
    flush_pending(my_conn);
    pthread_mutex_unlock(&my_conn->conn_mtx);

    while (true)
    {
        if (responder(my_conn, conns, page_mtx) < 0)
            break;
    }

    remove_conn(my_conn_id, conns, page_mtx);
    return NULL;
}
