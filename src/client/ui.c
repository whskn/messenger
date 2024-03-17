#include "ui.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>


/**
 * Get username
 * 
 * @param buffer where username will be written
 * @param out string to output before reading
 * 
 * @return 0 - name is in buffer;
 *         1 - syscall failed, check errno;
 *         2 - error reading, EOF;
 *         3 - inputed name is too long;
 *         4 - invalid name, only 0-9, A-Z, a-z;
*/
int get_username(username_t buffer, const char* out) {
    username_t tempBuffer;
    bzero(tempBuffer, sizeof(tempBuffer));

    system("clear");
    printf(out);
    fflush(stdout); // Force output

    struct pollfd fds = {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0};

    if ((poll(&fds, (nfds_t)1, -1)) < 0) {
        // TEHDOLG: error handling
        printf("polling failed...\n"); 
        return -1;
    }

    ssize_t bytesRead = read(STDIN_FILENO, tempBuffer, sizeof(username_t));
    if (bytesRead < 0) return 1;
    if (bytesRead == 0) return 2;

    unsigned short lastChar = bytesRead - 1;
    if (tempBuffer[lastChar] != '\n' && tempBuffer[lastChar] != '\0') {
        if (bytesRead == sizeof(tempBuffer)) return 3;
    } else {
        tempBuffer[lastChar] = '\0'; // replace possible \n with \0
    }

    for (int i = 0; i < bytesRead - 1; i++) {
        if ((tempBuffer[i] >= 48 && tempBuffer[i] <= 57) || // numbers
            (tempBuffer[i] >= 65 && tempBuffer[i] <= 90) || // capital letters
            (tempBuffer[i] >= 97 && tempBuffer[i] <= 122)) { // letters
                continue;
            }
        return 4;
    }

    memcpy(buffer, tempBuffer, sizeof(username_t));
    
    system("clear");

    return 0;
}


void print_error() {
    printf("\n\033[1;31mError:\033[0m %s\n", strerror(errno));
}