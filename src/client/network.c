#include <time.h>
#include <stdbool.h>
#include <poll.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "network.h"
#include "../misc/blocking_read.h"


/**
 * Creates a socket and tries to connect to a server.
 * 
 * @param ip ip address of the server
 * @param port port of the server
 * 
 * @return connection file descriptor or error code
*/
int tryConnect(const char* ip, const int port) {
    struct sockaddr_in address;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return NET_CHECK_ERRNO;
    }

    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    int err = connect(fd, (struct sockaddr*)&address, sizeof(address));
    if (err < 0) {
        close(fd);
        return NET_CHECK_ERRNO;
    } 

    return fd;
}

/**
 * Authenticates user on the server.
 * 
 * @param fd file descriptor of the tcp connection
 * @param username user's nickname
 * 
 * @return error codes
*/
int auth(int fd, username_t username) {
    int err;
    char code[HS_CODE_SIZE];

    err = write(fd, username, sizeof(username_t));
    if (err < 0) {
        return NET_CHECK_ERRNO;
    }

    err = blocking_read(fd, code, HS_CODE_SIZE, CONNECTION_TIMEOUT * 1000);
    if (err == BR_TIMEOUT) {
        return NET_SERVER_ERROR;
    }
    if (err == BR_CHECK_ERRNO) {
        return NET_CHECK_ERRNO;
    }
    if (err == BR_EOF) {
        return NET_CONN_DOWN;
    }
    
    // strcmp() returns 0 if strings are the same
    if (!strncmp(code, HS_SUCC, HS_CODE_SIZE))        return NET_SUCCESS          ;
    if (!strncmp(code, HS_INVAL_NAME, HS_CODE_SIZE))  return NET_INVALID_NAME     ;
    if (!strncmp(code, HS_USER_EXISTS, HS_CODE_SIZE)) return NET_USER_EXISTS      ;
    if (!strncmp(code, HS_MAX_CONN, HS_CODE_SIZE))    return NET_SERVER_OVERLOADED;

    return NET_SERVER_ERROR;
}

/**
 * Gathers tryConnect() and auth() together for cozy high-lever use.
 * 
 * @param c connection
 * @param ip ip of the server to connect to
 * @param port port of the server to connect to
 * 
 * @return error codes
*/
int clientConnect(connection_t* c, const char* ip, const int port) {
    int fd, ret;

    fd = tryConnect(ip, port);
    if (fd < 0) {
        return NET_CHECK_ERRNO;
    }

    ret = auth(fd, c->addr.from);
    if (ret == 0) {
        c->fd = fd;
    } 
    else {
        close(fd);
    }

    return ret;
}

/**
 * Closes connection with a server
 * 
 * @param c the connection to close
 * @return error codes
*/
int closeConn(connection_t* c) {
    return close(c->fd);
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
 * Checks message for validity.
 * @param size of msg excluding unused bytes in buffer
 * @param msg where the message is stored
 * 
 * @return weather the message valid or not
*/
bool message_is_valid(msg_t* msg, const int size) {
    int message_len = strnlen(msg->buffer, MAX_MESSAGE_SIZE);
    int from_len = strnlen(msg->names.from, sizeof(username_t));
    int to_len   = strnlen(msg->names.to,   sizeof(username_t));

    // negative checks
    if (size < MIN_MSG_SIZE || 
        size != (int)(msg->text_size + sizeof(msg_t) - sizeof(msg->buffer)) ||
        
        message_len == MAX_MESSAGE_SIZE ||
        // Last char of the message must always be \0. 
        // In this case message length == MAX_MESSAGE_SIZE, 
        // that leaves no place for \0.
        
        message_len < 1 ||
        msg->text_size < 2 ||
        // message len doesn't include \0, but text_size does

        from_len < 1 ||
        from_len > (int)sizeof(username_t) - 1 ||
        to_len < 1 ||
        to_len > (int)sizeof(username_t) - 1 ||
        // names are also C-strings, last char must be \0

        msg->timestamp == 0) {
            return false;
    } 
    return true;
}

/**
 * Send message from the buffer. Buffer can point to buffer of msg of connection
 * so that function won't copy the message. This can help avoid redunduncy.
 * 
 * @param c connection 
 * @param buffer message's buffer (better be pointed at c->msg->buffer)
 * @param length length of the message in buffer
 * 
 * @return size of message sent on success, error code otherwise
*/
int sendMessage(connection_t* c, msg_t* msg) {
    msg->text_size = strnlen(msg->buffer, MAX_MESSAGE_SIZE) + 1; // for \0
    msg->timestamp = time(NULL);
    memcpy(&(msg->names), &(c->addr), sizeof(c->addr));
    int msg_size = msg->text_size + sizeof(msg_t) - sizeof(msg->buffer);
    if (!message_is_valid(msg, msg_size)) {
        return NET_INVAL_MSG_FORMAT;
    }

    int err = write(c->fd, msg, msg_size);
    if (err < 0) {
        return NET_CHECK_ERRNO;
    }

    return msg_size;
}

/**
 * Read message from the socket and check it's format for validity.
 * 
 * @param fd file descriptor of the connection with a server.
 * @param msg message buffer to write message in.
 * 
 * @return size of read message or an error code
*/
int readMsg(connection_t* c, msg_t* msg) {
    int ret = read(c->fd, msg, sizeof(msg_t));
    if (ret < 0) {
        return NET_CHECK_ERRNO;
    }
    if (ret == 0) {
        return NET_CONN_DOWN;
    }

    if (!message_is_valid(msg, ret)) {
        return NET_INVAL_MSG_FORMAT;
    }

    return ret;
}
