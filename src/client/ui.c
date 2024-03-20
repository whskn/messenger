#include "ui.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define CLR_GOOD "\033[0;32m"
#define CLR_RED "\033[0;31m"
#define CLR_YELLOW "\033[0;33m"
#define CLR_RESET "\033[0m"

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
int getInput(char* buffer, unsigned len, const char* out) {
    char* tempBuffer = (char*)calloc(len, sizeof(char));

    system("clear");
    printf(out);
    fflush(stdout); // Force output

    struct pollfd fds = {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0};

    if ((poll(&fds, (nfds_t)1, -1)) < 0) {
        // TEHDOLG: error handling
        printf("polling failed...\n"); 
        free(tempBuffer);
        return -1;
    }

    ssize_t bytesRead = read(STDIN_FILENO, tempBuffer, len);
    if (bytesRead < 0) {
        free(tempBuffer);
        return 1;
    } if (bytesRead == 0) {
        free(tempBuffer);
        return 2;
    }

    unsigned short lastChar = bytesRead - 1;
    if (tempBuffer[lastChar] != '\n' && tempBuffer[lastChar] != '\0') {
        if (bytesRead == sizeof(tempBuffer)) {
            free(tempBuffer);
            return 3;
        }
    } else {
        tempBuffer[lastChar] = '\0'; // replace possible \n with \0
    }

    for (int i = 0; i < bytesRead - 1; i++) {
        if ((tempBuffer[i] >= 48 && tempBuffer[i] <= 57) || // numbers
            (tempBuffer[i] >= 65 && tempBuffer[i] <= 90) || // capital letters
            (tempBuffer[i] >= 97 && tempBuffer[i] <= 122)) { // letters
                continue;
            }
        free(tempBuffer);
        return 4;
    }

    memcpy(buffer, tempBuffer, len);
    
    system("clear");
    free(tempBuffer);
    return 0;
}


void print_error() {
    printf("\n\033[1;31mError:\033[0m %s\n", strerror(errno));
}


void printout_message(char* message, char* from, time_t timestamp) {
    // formatting timestamp to human-readable date
    char strTime[28];
    struct tm* timeinfo = localtime(&timestamp);
    sprintf(strTime, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

    printf("%s %s[%s]:%s %s\n", 
            strTime, 
            CLR_GOOD,
            from,
            CLR_RESET,
            message);
}