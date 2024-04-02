#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "serv.h"
#include "logger.h"

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
    if (openMainSocket(port, &sockFd) != NET_SUCCESS) {
        logger(LOG_ERROR, "Problem opening socket", true);
        exit(1);
    }

    // getting a mutex
    sem_t* mutex = (sem_t*)calloc(1, sizeof(sem_t));
    if (sem_init(mutex, 0, 1) < 0) {
        logger(LOG_ERROR, "Problem with sem_init", true);
        exit(1);
    }

    logger(LOG_GOOD, "Server is up!", false);
    for (;;) {
        int id = get_conn(sockFd, conns, mutex);
        if (id == NET_CHECK_ERRNO) {
            logger(LOG_ERROR, "Failed to harvest new connection", true);
            continue;
        } 
        else if (id == NET_AUTH_FAIL) {
            logger(LOG_ERROR, "Failed to establish connection", true);
            continue;
        } 

        logger(LOG_GOOD, "New connection!", false);

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

// TEHDOLG: WAIT FOR THREADS!!!