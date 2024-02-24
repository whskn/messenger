#include "network.h"

#define MAX_MESSAGE_LENGTH 1024

//TEHDOLG: with json or other type of cfg file
#define IP_ADDRESS "127.0.0.1"
#define PORT 6972


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: client [username]\n");
        return -1;
    }

    char msgBuffer[MAX_MESSAGE_LENGTH];
    int sockFd = 0;

    // checking and saving the username 
    username_t username;
    bzero(username, sizeof(username));
    
    size_t len = strlen(argv[1]);
    if (len > sizeof(username_t)) {
        printf("username's len must be <64 chars");
        exit(1);
    }
    memcpy(username, argv[1], len);

    for (;;) {
        if (tryConnect(IP_ADDRESS, PORT, &sockFd) != 0) {
            printf("Failed to connect...\n");
            continue;
        }
        if (auth(sockFd, username) < 0) {
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
