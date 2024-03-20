#include "network.h"
#include "ui.h"
#include "../history/history.h"

#include <pthread.h>
#include <errno.h>

#define DB_DIR "client_chats/"
#define IP "127.0.0.1"
#define PORT 6969

void manageConn(connection_t* c);

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
    while (getInput(c.addr.from, sizeof(username_t), 
           "ENTER YOUR USERNAME: ") != 0);
    while (getInput(c.addr.to, sizeof(username_t), 
           "WHO WOULD YOU LIKE TO CHAT WITH: ") != 0);

    // pulling message history
    int message_id = -1;
    int ret;
    while (true) {
        ret = history_read_next(DB_DIR, c.addr.to, (void*)c.msg, 
                                sizeof(msg_t), &message_id);
        if (ret != 0) {
            printf("SQLITE error code: %d\n", ret);
            return 1;
        }
        if (message_id == -1) break;
        printout_message(c.msg->buffer, c.msg->names.from, c.msg->timestamp);
    }

    // loop that re-tries to connect when conn breaks
    for (;;) {
        sleep(1);

        int ret = clientConnect(&c, ip, port);
        if (ret == 1) {
            printf("Failed to connect, errno: %s\n", strerror(errno));
        } else if (ret == 2) {
            printf("No response from the server...\n");
        } else if (ret == 3) {
            printf("Max number of connections on the server, try again later...\n");
        } else if (ret == 4) {
            printf("Invalid name...\n");
            exit(1);
        } else if (ret == 5) {
            printf("User already exists, choose another name...\n");
            exit(1);
        } if (ret > 0) continue;

        // message sender/reciever
        manageConn(&c);

        // close connection if manageConn returned
        closeConn(&c);
    }

    free(c.msg);

    return 0;
}




/**
 * Recieves and sends messages
 * 
 * @param c connection with the server
*/
void manageConn(connection_t* c) {
    // couple of shortcuts 
    msg_t* msg = c->msg;
    char* buff = msg->buffer;

    while (true) {
        // Blocking until message comes
        struct pollfd fds[2] = 
            {{.fd = c->fd, .events = POLLIN, .revents = 0},
            {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0}};

        int pollRet = poll(fds, (nfds_t)2, AFK_TIMEOUT * 1000);
        if (pollRet < 0) {
            printf("Failed to auth, errno: %s\n", strerror(errno));
        } else if (pollRet == 0) {
            printf("Timeout exceeded, no repond from the user...\n");
        }
        if (pollRet <= 0) {
            sleep(1);
            continue;
        }

        //checking what FD triggered poll to leave
        // incoming message
        if (fds[0].revents) {
            int readRet = readMsg(c);
            if (readRet == 1) {
                printf("Failed to read mgs, errno: %s\n", strerror(errno));
                break;
            } else if (readRet == 2) {
                printf("Connection broke...\n");
                break;
            } else if (readRet == 3) {
                printf("Recieved an invalid message...\n");
                continue;
            }
            
            int msg_size = msg->msg_size + sizeof(msg_t) - sizeof(msg->buffer);
            history_push(DB_DIR, c->addr.to, (void*)msg, msg_size);

            printout_message(buff, msg->names.from, msg->timestamp);
        } 

        // outcoming message
        if (fds[1].revents) {
            int readRet = read(fds[1].fd, buff, MAX_MESSAGE_LENGTH - 1);
            if (readRet < 0) {
                printf("Problem with read()...\n");
                break;
            } else if (readRet == 0) {
                printf("EOF...\n");
                continue;
            } else if (msg->buffer[0] == '\0') {
                continue;
            }

            // cause of \n
            if (buff[readRet - 1] == '\n') buff[readRet - 1] = '\0'; 
            else buff[readRet] = '\0';

            if (sendMessage(c, buff, readRet) > 0) {
                printf("Failed to send mgs, errno: %s\n", strerror(errno));
                break;
            }
            // TEHDOLG: INTERACTION THROUGH GLOBAL MSG STURCTURE FIXFIXFIX
            int msg_size = msg->msg_size + sizeof(msg_t) - sizeof(msg->buffer);
            history_push(DB_DIR, c->addr.to, (void*)msg, msg_size);
        }
    }

    return;
}

// TEHDOLG: VALIDATE MESSAGE CHECKS