#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "app.h"
#include "../history/history.h"

#define DB_DIR "client_chats/"

// pulling message history
int printout_history(username_t username) {
    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));
    int message_id = -1;
    int ret;

    while (true) {
        ret = history_read_next(DB_DIR, username, (void*)msg, 
                                sizeof(msg_t), &message_id);
        if (ret == HST_ERROR) {
            free(msg);
            return -1;
        }
        if (message_id == -1) {
            break;
        }

        if (!message_is_valid(msg, ret)) {
            printf("Invalid msg pulled from database\n");
            continue;
        } 

        printout_message(msg->buffer, msg->names.from, msg->timestamp);
    }
    free(msg);

    return 0;
}

/**
 * Recieves and sends messages
 * 
 * @param c connection with the server
*/
void manageConn(connection_t* c, msg_t* msg) {
    // couple of shortcuts 
    char* buff = msg->buffer;
    int ret = 0;

    while (true) {
        // polling input fd and socket fd
        struct pollfd fds[2] = 
            {{.fd = c->fd, .events = POLLIN, .revents = 0},
            {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0}};

        // Blocking until message comes
        ret = poll(fds, (nfds_t)2, AFK_TIMEOUT * 1000);
        if (ret < 0) {
            printf("Fail waiting for incoming messages: %s\n", strerror(errno));
            sleep(1);
            continue;
        } else if (ret == 0) {
            printf("Connection broke due to inactivity\n");
            break;
        }

        //checking what FD triggered poll to leave
        // incoming message
        if (fds[0].revents) {
            ret = readMsg(c, msg);
            if (ret == NET_CHECK_ERRNO) {
                printf("Failed to read mgs, errno: %s\n", strerror(errno));
                break;
            } else if (ret == NET_CONN_DOWN) {
                printf("Connection broke...\n");
                break;
            } else if (ret == NET_INVAL_MSG_FORMAT) {
                printf("Recieved an invalid message...\n");
                continue;
            }
            
            int text_size = msg->text_size + sizeof(msg_t) - sizeof(msg->buffer);
            ret = history_push(DB_DIR, c->addr.to, (void*)msg, text_size);
            if (ret != HST_SUCCESS) {
                printf("Failed to save message in database\n");
            }
            printout_message(buff, msg->names.from, msg->timestamp);
        } 

        // outcoming message
        if (fds[1].revents) {
            ret = read(fds[1].fd, buff, MAX_MESSAGE_SIZE - 1);
            if (ret < 0) {
                printf("Problem with read()...\n");
                break;
            } else if (ret == 0) {
                break;
            } else if (msg->buffer[0] == '\0') {
                continue;
            }
            int read_bytes = ret;

            // cause of \n
            if (buff[read_bytes - 1] == '\n') buff[read_bytes - 1] = '\0'; 
            else buff[read_bytes] = '\0';

            ret = sendMessage(c, msg);
            if (ret == NET_CHECK_ERRNO) {
                printf("Failed to send message, errno: %s\n", strerror(errno));
                break;
            }
            if (ret == NET_INVAL_MSG_FORMAT) {
                printf("Invalid message...\n");
                continue;
            } 

            int sent_bytes = ret;
            ret = history_push(DB_DIR, c->addr.to, (void*)msg, sent_bytes);
            if (ret != HST_SUCCESS) {
                printf("Failed to save message in database\n");
            }
        }
    }

    return;
}