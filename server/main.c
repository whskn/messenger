#include <stdio.h>
#include <stdbool.h>
#include "server.h"
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 1024
#define CHECK_TIMEOUT 1000
#define MAX_SERV_CONNECTIONS 512

//TEHDOLG: with json or other type of cfg file
#define PORT 6969


int main() {

    while(true) {
        conn_t connection;
        harvestConnections(PORT, &connection);

        pthread_create();
    }

    return 0;
}
