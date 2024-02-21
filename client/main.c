#include <stdio.h>
#include <stdbool.h>
#include "network.h"

#define MAX_MESSAGE_LENGTH 1024

//TEHDOLG: with json or other type of cfg file
#define IP_ADDRESS "127.0.0.1"
#define PORT 6969


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: client [username]\n");
        return -1;
    }

    char msgBuffer[MAX_MESSAGE_LENGTH];
    int sockFd = 0;

    for (;;) {
        if (tryConnect(IP_ADDRESS, PORT, &sockFd) != 0) {
            printf("Failed to connect...\n");
            continue;
        }
        username_t username;
        bzero(username, sizeof(username));
        memcpy(username, (char*)(argv[1]), strlen((char*)argv[1]));
        if (auth(sockFd, &username) < 0) {
            // TEHDOLG: error handling
            printf("auth failed...\n");
            return -1;
        }

        while (true) {
            printf("\n>> ");
            fgets(msgBuffer, MAX_MESSAGE_LENGTH, stdin);

            if (sendMessage(sockFd, msgBuffer, sizeof(msgBuffer)) < 0) {
                // TEHDOLG: error handling
                break;
            }
            close(sockFd);
        }
        printf("Problem with sendMessage, or connection broke...");
    }

    return 0;
}
