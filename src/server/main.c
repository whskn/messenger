#include <pthread.h>
#include <stdlib.h>

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


    //opening socket
    sem_t* mutex;
    conn_t* conns;
    int fd = serv_init(&conns, &mutex, PORT);
    if (fd < 0) exit(1);

    logger(LOG_GOOD, "Server is up!", false);
    for (;;) {
        int id = serv_get_conn(fd, conns, mutex);
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
        pthread_create(&thread, NULL, &serv_manage_conn, (void*)&args);
    }

    serv_close(fd, &conns, &mutex);
    return 0;
}

// TEHDOLG: WAIT FOR THREADS!!!