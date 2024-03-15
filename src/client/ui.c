#include "ui.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>


int get_username(username_t buffer, const char* out) {
    system("clear");
    printf(out);
    fflush(stdout); // Force output

    struct pollfd fds = {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0};

    if ((poll(&fds, (nfds_t)1, -1)) < 0) {
        // TEHDOLG: error handling
        printf("polling failed...\n"); 
        return -1;
    }

    ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(username_t));
    bytesRead--; // because of \n

    if (bytesRead < 1) {
        bzero(buffer, sizeof(username_t));
        return -1;
    }
    
    buffer[bytesRead] = '\0';
    system("clear");

    return 0;
}


void print_error() {
    printf("\n\033[1;31mError:\033[0m %s\n", strerror(errno));
}