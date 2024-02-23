#include <stdio.h>
#include <stdbool.h>
#include "network.h"
#include <pthread.h>

//TEHDOLG: with json or other type of cfg file
#define PORT 6969

int main() {
    conn_t* conns = (conn_t*)calloc(MAX_CONNECTIONS, sizeof(conn_t));
    for (int i = 0; i < MAX_CONNECTIONS; i++) conns[i].fd = -2;
    const int sockFd = openMainSocket(PORT);

    for (;;) {
        int connFd;
        if ((connFd = harvestConnection(sockFd)) < 0) {
            printf("Connection failed...\n");
            sleep(1); // remove
            continue;
        }

        if (authUser(connFd, conns) < 0) {
            printf("Connection unsuccessfull...\n");
            continue;
        }

        printf("Connection harvested\n");

        pthread_t thread; 
        MC_arg_t args = {.fd = &connFd, .conns = conns};
        pthread_create(&thread, 
                       NULL, 
                       &manageConnection, 
                       (void*)&args);

        while (connFd != EMPTY_FD) {
            sched_yield();
        }
    }

    return 0;
}
