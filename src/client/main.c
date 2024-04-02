#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "app.h"

#define IP "127.0.0.1"
#define PORT 6969

bool name_filter(char a) {
    if (!(a >= 48 && a <= 57) && 
        !(a >= 65 && a <= 90) &&
        !(a >= 97 && a <= 122)) {
            return false;
        }
    return true;
}

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
    
    msg_t* msgin = (msg_t*)calloc(1, sizeof(msg_t));
    msg_t* msgout = (msg_t*)calloc(1, sizeof(msg_t));

    username_t* chats = NULL;
    int n_chats = get_chats(chats);
    if (!chats) {
        // TEHDOLG
        return 1;
    }

    ui_t* ui_data = ui_init(MAX_MESSAGE_SIZE, (char*)chats, n_chats, 
                            sizeof(username_t));

    // saving the username and the reciever
    ui_get_input(ui_data, c.addr.from, sizeof(username_t), 
                 "ENTER YOUR USERNAME:", name_filter);
    ui_get_curr_chat(ui_data, c.addr.to);
    
    // launch main interface window
    void* args = {ui_data};
    pthread_t ui_thread; 
    pthread_create(&ui_thread, 
                    NULL, 
                    ui_handle, 
                    (void*)&args);

    // loop that re-tries to connect when conn breaks
    while (true) {
        sleep(1);

        int ret = clientConnect(&c, ip, port);
        if (ret == NET_CHECK_ERRNO) {
            printf("Failed to connect: %s\n", strerror(errno));
        } 
        else if (ret == NET_SERVER_ERROR) {
            printf("Server error...\n");
        } 
        else if (ret == NET_SERVER_OVERLOADED) {
            printf("Server is overloaded...\n");
        } 
        else if (ret == NET_INVALID_NAME) {
            printf("Invalid name...\n");
            free(msgin);
            free(msgout);
            return 1;
        }
        else if (ret == NET_USER_EXISTS) {
            printf("User already exists, choose another name...\n");
            free(msgin);
            free(msgout);
            return 1;
        } 
        else if (ret == NET_CONN_DOWN) {
            printf("Connection broke...\n");
        }

        if (ret != NET_SUCCESS) {
            continue;
        }

        // message sender/reciever
        manageConn(&c, ui_data, msgin, msgout);

        // close connection if manageConn returned
        closeConn(&c);
    }

    free(msgin);
    free(msgout);
    pthread_join(ui_thread, NULL);
    ui_close(ui_data);

    return 0;
}
