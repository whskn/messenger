#include "network.h"
#include <pthread.h>

//TEHDOLG: with json or other type of cfg file
#define PORT 6972

int main() {
    conn_t* conns = (conn_t*)calloc(MAX_CONNECTIONS, sizeof(conn_t));
    for (int i = 0; i < MAX_CONNECTIONS; i++) conns[i].fd = -2;
    const int sockFd = openMainSocket(PORT);

    sem_t* mutex = (sem_t*)calloc(1, sizeof(sem_t));
    if (sem_init(mutex, 0, 1) < 0) {
        printf("Problem with sem_init...\n");
        return -1;
    }

    for (;;) {
        int connFd;
        if ((connFd = harvestConnection(sockFd)) < 0) {
            printf("Connection failed...\n");
            sleep(1); // remove
            continue;
        }

        if (authUser(connFd, conns, mutex) < 0) {
            printf("Connection unsuccessfull...\n");
            continue;
        }

        printf("Connection harvested\n");

        pthread_t thread; 
        MC_arg_t args = {.fd = &connFd, .conns = conns, .mutex = mutex};
        pthread_create(&thread, 
                       NULL, 
                       &manageConnection, 
                       (void*)&args);

        while (connFd != EMPTY_FD) {
            sched_yield();
        }
    }

    sem_destroy(mutex);

    return 0;
}
