#include <time.h>

#include "network.h"
#include "../flags.h"

/**
 * Creates a socket and tries to connect to a server.
 * 
 * @param ip ip address of the server
 * @param port port of the server
 * 
 * @return 0 - success, 1 - check errno.
*/
int tryConnect(const char* ip, const int port, int* fd_ptr) {
    struct sockaddr_in address;
    int fd;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return 1;
    }

    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(fd);
        return 1;
    } 

    *fd_ptr = fd;

    return 0;
}

/**
 * Authenticates user on the server.
 * 
 * @param fd file descriptor of the tcp connection
 * @param username user's nickname
 * 
 * @return 0 - success;
 *         1 - check errno;
 *         2 - timeout exceeded;
 *         3 - max number of connections, try again later;
 *         4 - invalid name;
 *         5 - user with such name already exists;
 *         6 - invalid response from the server;
*/
int auth(int fd, username_t username) {
    if (write(fd, username, sizeof(username_t)) < 0) {
        return 1;
    }

    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    int pollRet = poll(&fds, (nfds_t)1, CONNECTION_TIMEOUT * 1000);

    if (pollRet < 0) return 1;
    if (pollRet == 0) return 2;

    char code[4];
    if (read(fd, &code, sizeof(hs_code_t)) < 0) {
        return 1;
    } 
    
    if (*(int*)code == *(int*)HS_SUCC)              return 0;
    else if (*(int*)code == *(int*)HS_MAX_CONN)     return 3;
    else if (*(int*)code == *(int*)HS_INVAL_NAME)   return 4;
    else if (*(int*)code == *(int*)HS_USER_EXISTS)  return 5;

    return 6;
}

/**
 * Closes connection with a server
 * 
 * @param c the connection to close
 * @return 0 - success, 1 - syscall failed, check errno
*/
int closeConn(connection_t* c) {
    if (close(c->fd) < 0) return 1;
    return 0;
}

/**
 * Gathers tryConnect() and auth() together for cozy high-lever use.
 * 
 * @param c connection
 * @param ip ip of the server to connect to
 * @param port port of the server to connect to
 * 
 * @return 0 - success;
 *         1 - check errno;
 *         2 - timeout exceeded;
 *         3 - max number of connections, try again later;
 *         4 - invalid name;
 *         5 - user with such name already exists; 
 *         6 - invalid response from the server
*/
int clientConnect(connection_t* c, const char* ip, const int port) {
    int tempFd;

    if (tryConnect(ip, port, &tempFd) != 0) return 1;

    int ret = auth(tempFd, c->addr.from);

    if (ret != 0) close(tempFd);
    else c->fd = tempFd;

    return ret;
}


/**
 * Send message from the buffer. Buffer can point to buffer of msg of connection
 * so that function won't copy the message. This can help avoid redunduncy.
 * 
 * @param c connection 
 * @param buffer message's buffer (better be pointed at c->msg->buffer)
 * @param length length of the message in buffer
 * 
 * @return 0 - success;
 *         1 - check errno, system call failed;
*/
int sendMessage(connection_t* c, char* buffer, size_t length) {
    memcpy(&(c->msg->names), &(c->addr), sizeof(c->addr));
    if (buffer != c->msg->buffer) {
        memcpy(c->msg->buffer, buffer, length);
    }
    c->msg->timestamp = time(NULL);
    c->msg->msg_size = length;

    size_t sizeof_msg = sizeof(*c->msg);
    ssize_t packet_size = c->msg->msg_size + sizeof_msg - sizeof(c->msg->buffer);

    if (write(c->fd, c->msg, packet_size) < 0) return 1;

    return 0;
}

/**
 * Read message from the socket and check it's format for validity.
 * 
 * @param fd file descriptor of the connection with a server.
 * @param msg message buffer to write message in.
 * @param me ...
 * 
 * @return 0 - success;
 *         1 - read error, check errno;
 *         2 - EOF, connection broke;
 *         3 - recieved an invalid message; 
*/
int readMsg(connection_t* c) {
    size_t sizeof_msg = sizeof(*c->msg);

    ssize_t ret = read(c->fd, c->msg, sizeof_msg);
    if (ret < 0) return 1;
    else if (ret == 0) return 2;

    if (ret < MIN_MESSAGE_LEN || 
        (size_t)ret != (c->msg->msg_size + sizeof_msg - sizeof(c->msg->buffer)) ||
        c->msg->msg_size < 1 ||
        *(c->msg->names.to) == '\0' ||
        *(c->msg->names.from) == '\0' ||
        c->msg->timestamp == 0 ||
        memcmp(c->msg->names.to, c->addr.from, sizeof(c->addr.from)) != 0) {
            return 3;
    }

    return 0;
}
