#include <poll.h>
#include <unistd.h>
#include "blocking_read.h"


/**
 * Blocks a thread until readed something from fd.
 * @param fd file descriptor
 * @param buffer buffer to put the readed something in
 * @param size size of buffer
 * @param timeout timelimit (-1 - no timelimit)
 * 
 * @return bytes read or erorr codes 
*/
int blocking_read(int fd, void* buffer, const int size, const int timeout) {
    // waiting for username
    int ret;
    struct pollfd fds = {.fd = fd, .events = POLLIN, .revents = 0};
    
    ret = poll(&fds, (nfds_t)1, timeout);
    if (ret < 0) {
        return BR_CHECK_ERRNO;
    }
    if (ret == 0) {
        return BR_TIMEOUT;
    }

    // reading username
    ret = read(fd, buffer, size);
    if (ret < 0) {
        return BR_CHECK_ERRNO;
    } 
    if (ret == 0) {
        return BR_EOF;
    }

    return ret;
}