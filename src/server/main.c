#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "serv.h"
#include "logger.h"
#include "config.h"

bool check_port(const char *port);

/*
These variables are global only to be accessed by int_handler()
(signal handler) function without passing an argument to it, since it's
not possible at all. Not reached from anywhere except int_handler()
*/
int fd;
mtx_t *page_mtx;
conn_t **page;

void int_handler()
{
    serv_close(fd, page, page_mtx);
    logger(LOG_GOOD, "Bye bye...", false);
    exit(0);
}

int main()
{
    signal(SIGINT, int_handler);
    signal(SIGKILL, int_handler);
    signal(SIGTERM, int_handler);
    signal(SIGABRT, int_handler);
    signal(SIGPIPE, SIG_IGN);

    int port;
    const char *port_str = getenv("SRV_PORT");
    if (port_str == NULL || !check_port(port_str))
    {
        logger(LOG_ERROR, "Missing or invalid SRV_PORT enviroment variable", false);
        exit(1);
    }
    port = atoi(port_str);
    if (port < 1024 || port > 65535)
    {
        logger(LOG_ERROR, "Invalid port", false);
        exit(1);
    }

    fd = serv_init(&page, &page_mtx, port);
    if (fd < 0)
    {
        exit(1);
    }

    logger(LOG_GOOD, "Server is up!", false);
    while (true)
    {
        int user_fd = serv_get_conn(fd);
        if (user_fd == NET_CHECK_ERRNO)
        {
            logger(LOG_ERROR, "Failed to harvest new connection", true);
            continue;
        }
        logger(LOG_INFO, "Connection harvested", false);

        pthread_t thread;
        MC_arg_t *args = (MC_arg_t *)malloc(sizeof(MC_arg_t));
        args->fd = user_fd;
        args->page = page;
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

    serv_close(fd, page, page_mtx);
    return 0;
}

bool check_port(const char *port)
{
    int port_valid;
    regex_t regex;

    const char *port_pattern = "^([1-5][0-9]{4}|[6-9][0-9]{3}|[1-9][0-9]{0,3})$";

    if (regcomp(&regex, port_pattern, REG_EXTENDED))
    {
        printf("Regex error\n");
        exit(1);
    }

    port_valid = regexec(&regex, port, 0, NULL, 0);
    regfree(&regex);
    if (port_valid < 0)
    {
        printf("Regex error\n");
        exit(1);
    }

    return !(bool)port_valid;
}