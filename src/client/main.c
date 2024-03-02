#include "network.h"
#include <pthread.h>

//TEHDOLG: with json or other type of cfg file
#define IP_ADDRESS "127.0.0.1"
#define PORT 6971


int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: client [your username] [reciever]\n");
        return -1;
    }

    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));

    // saving the username and the reciever
    fromto_t names;
    bzero(&names, sizeof(names));
    strcpy(names.from, argv[1]);
    strcpy(names.to, argv[2]);

    // loop that re-tries to connect when conn breaks
    int fd;
    for (;;) {
        if ((fd = clientConnect(IP_ADDRESS, PORT, names.from)) < 0) {
            sleep(CONNECTION_TIMEOUT);
            continue;
        }

        struct pollfd fds[2] = 
            {{.fd = fd, .events = POLLIN, .revents = 0},
            {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0}};

        while (true) {
            // Blocking until message comes
            if ((poll(fds, (nfds_t)2, AFK_TIMEOUT * 1000)) < 0) {
                // TEHDOLG: error handling
                printf("polling failed...\n"); 
                break;
            }

            if (fds[0].revents) {
                int readRet = readMsg(fds[0].fd, msg, names.from);
                if (readRet < 0) {
                    printf("Problem with readMsg()...\n");
                    break;
                }

                write(STDIN_FILENO, msg->buffer, msg->msg_size);
                printf("\n");
                fds[0].revents = 0;
            } 

            if (fds[1].revents) {
                int readRet = read(fds[1].fd, msg->buffer, sizeof(msg->buffer));
                if (readRet < 0) {
                    printf("Problem with read()...\n");
                    continue;
                } else if (readRet == 0) {
                    printf("EOF...\n");
                    continue;
                } else if (msg->buffer[0] == '\0') {
                    continue;
                }
                
                // building the message structure
                readRet--; // cause of \n
                memcpy(&msg->names, &names, sizeof(names));
                msg->timestamp = time(NULL);
                msg->msg_size = readRet;

                if (sendMessage(fd, msg) < 0) {
                    // TEHDOLG: error handling
                    close(fd);
                    break;
                }
                fds[1].revents = 0;
            }
        }
        printf("Problem with sendMessage, or connection broke...\n");
    }

    return 0;
}
