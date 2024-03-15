#include "network.h"
#include "ui.h"
#include <pthread.h>
#include <errno.h>

#define IP "127.0.0.1"
#define PORT 6969

int main() {
    // Getting ENVs
    // const char* ip = getenv("ip");
    // const int port = atoi(getenv("port"));

    const char* ip = IP;
    const int port = PORT;
    if (ip == NULL || port == 0) {
        printf("Missing ip or port env variable...\n");
        return -1;
    } else if (port < 1024 || port > 65535) {
        printf("Invalid port...\n");
        return -1;
    }


    // allocating mem for structures used to send messages 
    connection_t c;
    c.msg = (msg_t*)calloc(1, sizeof(msg_t));

    // saving the username and the reciever
    while (get_username(c.addr.from, "ENTER YOUR USERNAME: ") != 0);
    while (get_username(c.addr.to, "WHO WOULD YOU LIKE TO CHAT WITH: ") != 0);


    // loop that re-tries to connect when conn breaks
    for (;;) {
        //TEHDOLG
        int ret = clientConnect(&c, ip, port);

        struct pollfd fds[2] = 
            {{.fd = c.fd, .events = POLLIN, .revents = 0},
            {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0}};

        while (true) {
            // Blocking until message comes
            if ((poll(fds, (nfds_t)2, AFK_TIMEOUT * 1000)) < 0) {
                print_error();
                break;
            }

            if (fds[0].revents) {
                int readRet = readMsg(&c);
                if (readRet < 0) {
                    printf("Problem with readMsg()...\n");
                    break;
                }

                write(STDIN_FILENO, c.msg->buffer, c.msg->msg_size);
                printf("\n");
                fds[0].revents = 0;
            } 

            if (fds[1].revents) {
                int readRet = read(fds[1].fd, c.msg->buffer, sizeof(c.msg->buffer));
                // TEHDOLG
                if (readRet < 0) {
                    printf("Problem with read()...\n");
                    break;
                } else if (readRet == 0) {
                    printf("EOF...\n");
                    continue;
                } else if (c.msg->buffer[0] == '\0') {
                    continue;
                }
                
                readRet--; // cause of \n
                switch (sendMessage(&c, c.msg->buffer, readRet)) {
                    case 1:
                        // TEHDOLG
                        printf("smth with sendmessage\n");
                        break;
                }
                
                fds[1].revents = 0;
            }
        }

        closeConn(&c);
        printf("Problem with sendMessage, or connection broke...\n");
    }

    return 0;
}
