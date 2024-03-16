#include "network.h"
#include "logger.h"
#include <pthread.h>

#define PORT 6969

int main() {
    // Getting ENVs

    const int port = PORT;
    // const int port = atoi(getenv("port"));
    if (port == 0) {
        logger(LOG_ERROR, "Missing ip or port env variable", false);
        return -1;
    } else if (port < 1024 || port > 65535) {
        logger(LOG_ERROR, "Invalid port", false);
        return -1;
    }

    // allocating mem for connections
    conn_t* conns = (conn_t*)calloc(MAX_CONNECTIONS, sizeof(conn_t));
    for (int i = 0; i < MAX_CONNECTIONS; i++) conns[i].fd = -2;
    
    //opening socket
    int sockFd;
    if (openMainSocket(port, &sockFd) != 0) {
        logger(LOG_ERROR, "Problem opening socket", true);
        exit(1);
    }

    // getting a mutex
    sem_t* mutex = (sem_t*)calloc(1, sizeof(sem_t));
    if (sem_init(mutex, 0, 1) < 0) {
        logger(LOG_ERROR, "Problem with sem_init", true);
        exit(1);
    }

    for (;;) {
        int connFd = EMPTY_FD;
        if (harvestConnection(sockFd, &connFd) != 0) {
            logger(LOG_WARNING, "Error while accepting a connection", true);
            continue;
        }

        int id = 3;
        int authRet = authUser(connFd, &id, conns, mutex);
        if (authRet == 1) {
            logger(LOG_WARNING, "Failed to auth user", true);
        } else if (authRet == 2) {
            logger(LOG_INFO, "No response from the user", false);
        } else if (authRet == 3) {
            logger(LOG_INFO, "Auth unsuccessfull", false);
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
