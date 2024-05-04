#include <time.h>
#include <poll.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "network.h"
#include "../misc/validate.h"

static int try_connect(const char *ip, const int port)
{
    struct sockaddr_in address;
    struct pollfd fds;
    int err;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0)
    {
        return NET_CHECK_ERRNO;
    }

    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    err = connect(fd, (struct sockaddr *)&address, sizeof(address));
    if (err < 0 && errno != EINPROGRESS)
    {
        close(fd);
        return NET_CHECK_ERRNO;
    }

    fds.fd = fd;
    fds.events = POLLOUT;

    err = poll(&fds, 1, CONNECTION_TIMEOUT);
    if (err == -1)
    {
        close(fd);
        return NET_CHECK_ERRNO;
    }
    else if (err == 0)
    {
        close(fd);
        return NET_TIMEOUT;
    }
    else if (!(fds.revents & POLLOUT))
    {
        close(fd);
        return NET_SERVER_ERROR;
    }

    return fd;
}

static int auth(int fd, username_t username, password_t password,
                const bool new_account)
{
    int ret;
    const int auth_type = new_account ? AUTH_REGISTER : AUTH_LOGIN;
    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    auth_res_t rsp;
    auth_req_t req;

    req.cc = CC_AUTH;
    req.auth_type = auth_type;
    memcpy(req.username, username, sizeof(username_t));
    memcpy(req.password, password, sizeof(username_t));

    ret = net_send(fd, &req, sizeof(auth_req_t));

    ret = poll(&fds, (nfds_t)1, AUTH_TIMEOUT);
    if (ret < 0)
        return NET_SERVER_ERROR;

    ret = net_read(fd, &rsp, sizeof(auth_res_t));
    if (ret < 0)
        return NET_SERVER_ERROR;

    switch (rsp.hs_code)
    {
    case HS_SUCC:
        return rsp.user_id;
    case HS_MAX_CONN:
        return NET_SERVER_OVERLOADED;
    case HS_INVAL_AUTH:
        return NET_INVALID_AUTH;
    case HS_USER_EXISTS:
        return NET_USER_EXISTS;
    case HS_GENERIC_ERROR:
        return NET_SERVER_ERROR;
    case HS_NO_USER:
        return NET_NO_USER;
    }

    return NET_SERVER_ERROR;
}

int net_connect(connection_t *c, const char *ip, const int port,
                username_t my_name, password_t password, const bool new_acc)
{
    int fd, ret, user_id;

    fd = try_connect(ip, port);
    if (fd < 0)
    {
        return NET_CHECK_ERRNO;
    }

    ret = auth(fd, my_name, password, new_acc);
    if (ret < 0)
    {
        close(fd);
        return ret;
    }

    c->fd = fd;
    user_id = ret;
    strncpy(c->my_name, my_name, USERNAME_LEN);
    c->my_id = user_id;

    return user_id;
}

int net_user_req(connection_t *c, username_t name)
{
    user_req_t req;
    req.cc = CC_USER_RQS;
    memcpy(req.username, name, sizeof(username_t));

    if (net_send(c->fd, &req, sizeof(user_req_t)) < 0)
    {
        return NET_CHECK_ERRNO;
    }

    return NET_SUCCESS;
}

int net_close_conn(connection_t *c)
{
    return !close(c->fd) ? NET_SUCCESS : NET_CHECK_ERRNO;
}

int net_send(const int fd, void *buffer, const int size)
{
    int ret;

    ret = write(fd, &size, sizeof(int));
    if (ret != sizeof(int))
    {
        return NET_ERROR;
    }

    ret = write(fd, buffer, size);
    if (ret < 0)
    {
        return NET_CHECK_ERRNO;
    }

    return ret;
}

int net_send_msg(connection_t *c,
                 msg_t *msg,
                 char *buffer,
                 const int to_id)
{
    if (buffer != msg->buffer)
    {
        strncpy(msg->buffer, buffer, MAX_MESSAGE_LEN);
    }

    msg->cc = CC_MSG;
    msg->from_id = c->my_id;
    msg->to_id = to_id;
    strncpy(msg->from_name, c->my_name, USERNAME_LEN);
    msg->text_size = strnlen(msg->buffer, MAX_MESSAGE_LEN) + 1; // for \0
    msg->timestamp = time(NULL);

    int msg_size = msg_size(msg);
    if (!msg_is_valid((void *)msg, msg_size))
    {
        return NET_INVAL_MSG_FORMAT;
    }

    return net_send(c->fd, msg, msg_size);
}

int net_read(const int fd, void *buffer, const int size)
{
    int ret;
    int packet_size;

    ret = read(fd, &packet_size, sizeof(int));
    if (ret != sizeof(int))
        return NET_ERROR;

    if (packet_size > size)
        return NET_ERROR;

    ret = read(fd, buffer, packet_size);
    if (ret < 0)
    {
        return NET_ERROR;
    }
    if (ret == 0)
    {
        return NET_CONN_DOWN;
    }

    return ret;
}

int net_flush(connection_t *c)
{
    const int sizeof_buffer = 256;
    void *buffer = (void *)malloc(sizeof_buffer);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    while (read(c->fd, buffer, sizeof_buffer) > 0)
        ;
    fcntl(STDIN_FILENO, F_SETFL, flags);

    free(buffer);
    return NET_SUCCESS;
}