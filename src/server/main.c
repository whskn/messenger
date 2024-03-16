#include "network.h"
#include <pthread.h>
#include <errno.h>

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
    int sockFd;
    if (openMainSocket(port, &sockFd) != 0) {
        printf("Problem opening socket: %s\n", strerror(errno));
        exit(1);
    }

    // getting a mutex
    sem_t* mutex = (sem_t*)calloc(1, sizeof(sem_t));
    if (sem_init(mutex, 0, 1) < 0) {
        printf("Problem with sem_init...\n");
        exit(1);
    }

    for (;;) {
        int connFd = EMPTY_FD;
        if (harvestConnection(sockFd, &connFd) != 0) {
            printf("Failed to harvest, errno: %s\n", strerror(errno));
            continue;
        }

        int id = 3;
        int authRet = authUser(connFd, &id, conns, mutex);
        if (authRet == 1) {
            printf("Failed to auth, errno: %s\n", strerror(errno));
        } else if (authRet == 2) {
            printf("Connection timed out, no response from the user...\n");
        } else if (authRet == 3) {
            printf("Auth unsuccessfull...\n");
        } if (authRet > 0) continue;

        pthread_t thread; 
        MC_arg_t args = {.id = id, .conns = conns, .mutex = mutex};
        pthread_create(&thread, 
                       NULL, 
                       &manageConnection, 
                       (void*)&args);
    }

    sem_destroy(mutex);
    free(conns);
    free(mutex);
    close(sockFd);

    return 0;
}
