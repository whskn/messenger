#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "../history/history.h"
#include "../misc/validate.h"

#define DB_DIR "client_chats/"

bool name_filter(char a) {
    if (!(a >= 48 && a <= 57) && 
        !(a >= 65 && a <= 90) &&
        !(a >= 97 && a <= 122)) {
            return false;
        }
    return true;
}

int get_chats(username_t** chats) {
    DIR *dir;
    struct dirent *entry;
    int filecount = 0;

    // Open the directory
    dir = opendir(DB_DIR);
    if (dir == NULL) {
        // TEHDOLG
        return -1;
    }

    // counting number of entities in the directory
    for (; readdir(dir) != NULL; filecount++);

    *chats = (username_t*)calloc(sizeof(username_t), filecount);

    rewinddir(dir);
    // Read each entry in the directory
    int ext_len = strlen(EXTENSION);
    int filei = 0;
    for (; (entry = readdir(dir)) != NULL && filei < filecount;) {
        int namelen = strlen(entry->d_name) - ext_len;

        // checking if the filename is valid
        if (namelen < 1 || 
            strcmp(entry->d_name + namelen, EXTENSION) ||
            namelen > (int)sizeof(username_t) - 1) {  // -1 for \0
            continue;
        }

        memcpy(*chats[filei], entry->d_name, namelen);
        *chats[filei][namelen] = '\0';
        filei++;
    }

    closedir(dir);
    return filei;
}

// pulling message history
int load_history(ui_t* ui_data, const char* myname) {
    if (CURR_CHAT(ui_data) == NULL) return 0;

    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));
    int ret;

    username_t username;
    ui_get_curr_chat(ui_data, username);
    if (username[0] == '\0') {
        // ui_data
        return 0;
    }

    int message_id = -1;
    while (true) {
        // TEHDOLG pull only last hist_len but not the whole db
        ret = history_read_next(DB_DIR, username, (void*)msg, 
                                sizeof(msg_t), &message_id);
        if (ret == HST_ERROR) {
            free(msg);
            return -1;
        }
        if (message_id == -1) {
            break;
        }

        if (!msg_is_valid((void*)msg, ret)) {
            printf("Invalid msg pulled from database\n");
            continue;
        } 

        int msg_len = msg->text_size - 1; // because of \0
        char* from = (strcmp(msg->names.from, myname)
                          ? msg->names.from : ME_NAMETAG);
        ui_append_message(ui_data, msg->timestamp, msg->buffer, msg_len, from);
    }
    free(msg);

    return 0;
}

/**
 * Recieves and sends messages
 * 
 * @param c connection with the server
*/
void manageConn(connection_t* c, ui_t* ui_data, msg_t* msgin, msg_t* msgout) {
    // couple of shortcuts 
    int ret = 0;

    while (true) {

        // polling input fd and socket fd
        struct pollfd fds = {.fd = c->fd, .events = POLLIN, .revents = 0};

        // Blocking until message comes
        ret = poll(&fds, (nfds_t)1, 50);
        if (ret < 0) {
            printf("Fail waiting for incoming messages: %s\n", strerror(errno));
            sleep(1);
            continue;
        }

        //checking what FD triggered poll to leave
        // incoming message
        if (fds.revents) {
            ret = readMsg(c, msgin);
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

            ui_append_message(ui_data, msgin->timestamp, msgin->buffer, 
                              msgin->text_size, msgin->names.to);
            ret = history_push(DB_DIR, c->addr.to, (void*)msgin, msg_size(msgin));
            if (ret != HST_SUCCESS) {
                printf("Failed to save message in database\n");
            }
        }

        // interface call
        if (ui_new_message(ui_data)) {
            ret = sendMessage(c, msgout, ui_get_buffer(ui_data));
            if (ret == NET_CHECK_ERRNO) {
                printf("Failed to send message, errno: %s\n", strerror(errno));
                break;
            }
            if (ret == NET_INVAL_MSG_FORMAT) {
                printf("Invalid message...\n");
                continue;
            }

            int sent_bytes = ret;
            ui_append_message(ui_data, msgout->timestamp, msgout->buffer, 
                                msgout->text_size, ME_NAMETAG);
            ret = history_push(DB_DIR, c->addr.to, (void*)msgout, sent_bytes);
            if (ret != HST_SUCCESS) {
                printf("Failed to save message in database\n");
            }
            ui_clear_buffer(ui_data);
        }
        if (ui_chat_switch(ui_data)) {
            char* chat = CURR_CHAT(ui_data);
            if (chat) {
                strcpy(c->addr.to, chat);
                load_history(ui_data, c->addr.from);
            }
        }
        if (ui_new_chat(ui_data)) {
            username_t newchat;
            ui_get_input(ui_data, newchat, sizeof(username_t), 
                         "NAME OF THE NEW USER: ", name_filter);
            ui_add_chat(ui_data, newchat);
            char* chat = CURR_CHAT(ui_data);
            if (chat) {
                strcpy(c->addr.to, chat);
                load_history(ui_data, c->addr.from);
            }
        }

        ui_reset_code(ui_data);
    }

    return;
}