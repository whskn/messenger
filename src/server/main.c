#include "network.h"
#include <pthread.h>

#define PORT 6969

int main() {
    // Getting ENVs

    const int port = PORT;
    // const int port = atoi(getenv("port"));
    if (port == 0) {
        printf("Missing ip or port env variable...\n");
        return -1;
    } else if (port < 1024 || port > 65535) {
        printf("Invalid port...\n");
        return -1;
    }

    // allocating mem for connections
    conn_t* conns = (conn_t*)calloc(MAX_CONNECTIONS, sizeof(conn_t));
    for (int i = 0; i < MAX_CONNECTIONS; i++) conns[i].fd = -2;
    
    //opening socket
    const int sockFd;
    if (openMainSocket(port, &sockFd) != 0) {
        //TEHDOLG
        printf("Problem with opening the socket...\n");
        exit(1);
    }

    sem_t* mutex = (sem_t*)calloc(1, sizeof(sem_t));
    if (sem_init(mutex, 0, 1) < 0) {
        printf("Problem with sem_init...\n");
        exit(1);
    }

    for (;;) {
        int connFd = EMPTY_FD;
        if (harvestConnection(sockFd, &connFd) != 0) {
            printf("Connection failed...\n");
            sleep(1); // remove
            continue;
        }

        int id;
        if ((id = authUser(connFd, conns, mutex)) < 0) {
            printf("Connection unsuccessfull...\n");
            continue;
        }

        printf("Connection harvested\n");

        pthread_t thread; 
        MC_arg_t args = {.id = id, .conns = conns, .mutex = mutex};
        pthread_create(&thread, 
                       NULL, 
                       &manageConnection, 
                       (void*)&args);
    }

    sem_destroy(mutex);

    return 0;
}
