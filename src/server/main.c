#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "serv.h"
#include "logger.h"
#include "config.h"

#define PORT 6969

int fd;
mtx_t *page_mtx;
conn_t **conns;

void int_handler()
{
    serv_close(fd, conns, page_mtx);
    logger(LOG_GOOD, "Bye bye...", false);
    exit(0);
}

int main()
{
    // Getting ENVs
    signal(SIGINT, int_handler);
    signal(SIGKILL, int_handler);
    signal(SIGTERM, int_handler);
    signal(SIGABRT, int_handler);
    signal(SIGPIPE, SIG_IGN);

    const int port = PORT;
    // const int port = atoi(getenv("port"));
    if (port == 0)
    {
        logger(LOG_ERROR, "Missing ip or port env variable", false);
        return -1;
    }
    else if (port < 1024 || port > 65535)
    {
        logger(LOG_ERROR, "Invalid port", false);
        return -1;
    }

    fd = serv_init(&conns, &page_mtx, PORT);
    if (fd < 0)
        exit(1);

    logger(LOG_GOOD, "Server is up!", false);
    while (true)
    {
        int user_fd = serv_get_conn(fd);
        if (user_fd == NET_CHECK_ERRNO)
        {
            logger(LOG_ERROR, "Failed to harvest new connection", true);
            continue;
        }

        logger(LOG_GOOD, "New connection!", false);

        pthread_t thread;
        MC_arg_t *args = (MC_arg_t *)malloc(sizeof(MC_arg_t));
        args->fd = user_fd;
        args->conns = conns;
        args->page_mtx = page_mtx;

        if (pthread_create(&thread, NULL, &serv_manage_conn, (void *)args))
        {
            logger(LOG_ERROR, "Failed to create a thread", false);
        }
        else if (pthread_detach(thread))
        {
            logger(LOG_ERROR, "Failed to detach a thread, terminating it...", false);
            pthread_cancel(thread);
        }
    }

    serv_close(fd, conns, page_mtx);
    return 0;
}
