#include "ui.h"
#include "app.h"

#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define IP "127.0.0.1"
#define PORT 6969

void manageConn(connection_t* c, msg_t* msg);

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
    msg_t* msg = (msg_t*)calloc(1, sizeof(msg_t));

    // saving the username and the reciever
    while (getInput(c.addr.from, sizeof(username_t), 
           "ENTER YOUR USERNAME: ") != 0);
    while (getInput(c.addr.to, sizeof(username_t), 
           "WHO WOULD YOU LIKE TO CHAT WITH: ") != 0);

    printout_history(c.addr.to);

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
            free(msg);
            return 1;
        } 
        else if (ret == NET_USER_EXISTS) {
            printf("User already exists, choose another name...\n");
            free(msg);
            return 1;
        } 
        else if (ret == NET_CONN_DOWN) {
            printf("Connection broke...\n");
        }

        if (ret != NET_SUCCESS) {
            continue;
        }

        // message sender/reciever
        manageConn(&c, msg);

        // close connection if manageConn returned
        closeConn(&c);
    }

    free(msg);

    return 0;
}