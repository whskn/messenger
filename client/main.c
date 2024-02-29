#include "network.h"
#include "interface.h"
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 1024

//TEHDOLG: with json or other type of cfg file
#define IP_ADDRESS "127.0.0.1"
#define PORT 6969


int main(int argc, char* argv[]) {
    init_ncur();
    draw_top_bar();

    if (argc < 2) {
        end_ncur();
        printf("Usage: client [username]\n");
        return -1;
    }

    char msgBuffer[MAX_MESSAGE_LENGTH];
    int fd = 0;

    // checking and saving the username 
    username_t username;
    bzero(username, sizeof(username));
    
    size_t len = strlen(argv[1]);
    if (len > sizeof(username_t)) {
        printf("username's len must be <64 chars");
        exit(1);
    }
    memcpy(username, argv[1], len);

    // loop that re-tries to connect when conn breaks
    for (;;) {
        if (tryConnect(IP_ADDRESS, PORT, &fd) != 0) {
            printf("Failed to connect...\n");
            sleep(CONNECTION_TIMEOUT);
            continue;
        }
        if (auth(fd, username) < 0) {
            // TEHDOLG: error handling
            end_ncur();
            printf("auth failed...\n");
            return -1;
        }

        username_t reciever = "bill";
        struct pollfd fds[2];

        fds[0].fd = fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        fds[1].fd = STDIN_FILENO;
        fds[1].events = POLLIN; 
        fds[1].revents = 0;

        while (true) {
            printf("\r>> ");

            // Blocking until message comes
            if ((poll(fds, (nfds_t)2, AFK_TIMEOUT * 1000)) < 0) {
                // TEHDOLG: error handling
                printf("polling failed...\n"); 
                break;
            }

            if (fds[0].revents) {
                username_t from;
                int readRet = readMsg(fd, from, msgBuffer, sizeof(msgBuffer));
                if (readRet < 0) {
                    printf("Problem with readMsg()...\n");
                    break;
                }

                printf("\r<< %s \n", msgBuffer);
                fds[0].revents = 0;
            } 
            if (fds[1].revents) {
                fgets(msgBuffer, MAX_MESSAGE_LENGTH, stdin);
                if (sendMessage(fd, reciever, msgBuffer, sizeof(msgBuffer)) < 0) {
                    // TEHDOLG: error handling
                    close(fd);
                    break;
                }
                fds[1].revents = 0;
            }
        }
        printf("Problem with sendMessage, or connection broke...\n");
    }

    end_ncur();
    return 0;
}
