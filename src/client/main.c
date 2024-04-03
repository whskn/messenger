#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "app.h"


#define MAX_ADDR_SIZE sizeof("255.255.255.255:65535")
#define ADDR_INPUT "ENTER SERVER's IP AND PORT (IP:PORT) :"
#define USERNAME_INPUT "ENTER YOUR USERNAME:"

#define ADDR "127.0.0.1:6969"

bool name_filter(char a);
bool address_filter(char a);
int parse_address(char* addr);


int main() {
    username_t* chats = NULL;
    int n_chats = get_chats(&chats);
    if (n_chats < 0) {
        // TEHDOLG
        return 1;
    }

    ui_t* ui_data = ui_init(MAX_MESSAGE_SIZE, (char*)chats, n_chats, 
                            sizeof(username_t));
    

    // asking for ip:port of a server
    char addr[MAX_ADDR_SIZE] = ADDR;
    int port;
    while (true) {
        // ui_get_input(ui_data, addr, MAX_ADDR_SIZE, ADDR_INPUT, address_filter);
        port = parse_address(addr);

        // TEHDOLG proper check
        if (port < 0) {
            printf("Missing ip or port env variable...\n");
            continue;
        } else if (port < 1024 || port > 65535) {
            printf("Invalid port...\n");
            continue;
        }
        
        break;
    }


    // allocating mem for structures used to send messages 
    connection_t c;
    msg_t* msgin  = (msg_t*)calloc(1, sizeof(msg_t));
    msg_t* msgout = (msg_t*)calloc(1, sizeof(msg_t));

    // saving the username and the reciever
    ui_get_input(ui_data, c.addr.from, sizeof(username_t), USERNAME_INPUT, 
                 name_filter);
    ui_get_curr_chat(ui_data, c.addr.to);
    
    // loading message history
    load_history(ui_data, c.addr.from);

    // launch main interface window
    void* args = {ui_data};
    pthread_t ui_thread; 
    pthread_create(&ui_thread, NULL, ui_handle, args);


    // loop that re-tries to connect when conn breaks
    while (true) {
        sleep(1);

        int ret = clientConnect(&c, addr, port);
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

bool address_filter(char a) {
    if ((a >= 48 || a <= 57) || a == '.' || a == ':') return true;
    return false;
}

// cut off port and return it as an int
int parse_address(char* addr) {
    char* _ptr = addr; 
    for (; *_ptr != ':'; _ptr++) if (*_ptr == '\0') return -1;
    *_ptr++ = '\0';
    return *_ptr >= 48 || *_ptr <= 57 ? atoi(_ptr) : -1;
}