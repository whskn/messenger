#include <stdio.h>
#include <stdbool.h>
#include "network.h"

#define MAX_MESSAGE_LENGTH 1024
#define CONNECTION_TIMEOUT 5000

//TEHDOLG: with json or other type of cfg file
#define IP_ADRESS "172.0.0.1"
#define PORT 6969

int blockingConnect(const char* ip, 
                    const int port, 
                    const int* fd_ptr, 
                    short timeout);

int main() {
    char msgBuffer[MAX_MESSAGE_LENGTH];
    int sockFd;

    blockingConnect(IP_ADRESS, PORT, &sockFd, CONNECTION_TIMEOUT);

    while (true) {
        printf("\n>> ");
        if (fgets(msgBuffer, MAX_MESSAGE_LENGTH, stdin) < 0) {
            perror("Problem with fgets()");
            return 1;
        }

        if (sendMessage(sockFd, msgBuffer, sizeof(msgBuffer)) < 0) {
            // TEHDOLG: error handling
            return 1;
        }
    }

    return 0;
}


int blockingConnect(const char* ip, 
                    const int port, 
                    const int* fd_ptr, 
                    short timeout) {
    while (true) {
        if (tryConnect(ip, port, fd_ptr) == 0) {
            return 0;
        }
        else {
            // TEHDOLG: error handling
            printf("Failed to connect...");
            return 1;
        }

        sleep(timeout);
    }
}