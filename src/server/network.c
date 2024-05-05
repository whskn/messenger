#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "network.h"

int net_open_sock(const int port)
{
    struct sockaddr_in address;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return NET_CHECK_ERRNO;
    }

    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(fd, (struct sockaddr *)&address, sizeof(address))) != 0)
    {
        close(fd);
        return NET_CHECK_ERRNO;
    }

    if ((listen(fd, CONN_QUEUE)) != 0)
    {
        close(fd);
        return NET_CHECK_ERRNO;
    }

    return fd;
}

int net_harvest_conn(const int sock_fd)
{
    struct sockaddr_in cli;
    int fd;
    socklen_t cliLen = sizeof(struct sockaddr_in);

    fd = accept(sock_fd, (struct sockaddr *)&cli, &cliLen);
    if (fd < 0)
    {
        return NET_CHECK_ERRNO;
    }

    return fd;
}

int net_close_conn(const int fd)
{
    if (shutdown(fd, SHUT_RDWR) < 0)
    {
        return NET_CHECK_ERRNO;
    }
    if (close(fd) < 0)
    {
        return NET_CHECK_ERRNO;
    }
    return NET_SUCCESS;
}

int net_send(int fd, void *buffer, const int size)
{
    int ret;

    ret = write(fd, &size, sizeof(int));
    if ((size_t)ret < sizeof(int))
    {
        return NET_CONN_BROKE;
    }

    ret = write(fd, buffer, size);
    if (ret < 0)
    {
        return NET_CHECK_ERRNO;
    }
    if (ret == 0)
    {
        return NET_CONN_BROKE;
    }

    return NET_SUCCESS;
}

int net_read(const int fd, void *buffer, const int size)
{
    int ret;
    int packet_size;

    ret = read(fd, &packet_size, sizeof(int));
    if (ret < 0)
    {
        return NET_ERROR;
    }
    if (ret == 0)
    {
        return NET_CONN_BROKE;
    }

    if (packet_size > size)
    {
        return NET_ERROR;
    }

    ret = read(fd, buffer, packet_size);
    if (ret < 0)
    {
        return NET_ERROR;
    }
    if (ret == 0)
    {
        return NET_CONN_BROKE;
    }

    return ret;
}

// int net_read(const int fd, void *buffer, const int size)
// {
//     int ret;

//     int data_size;
//     int sz_bytes_read = 0;
//     int data_bytes_read = 0;

//     do
//     {
//         ret = read(fd, &data_size, sizeof(int) - sz_bytes_read);
//         if (!ret)
//         {
//             return NET_CONN_BROKE;
//         }
//         if (ret == -1)
//         {
//             return NET_ERROR;
//         }
//     } while ((sz_bytes_read += ret) < (int)sizeof(int));

//     if (data_size > size)
//     {
//         return NET_ERROR;
//     }

//     do
//     {
//         ret = read(fd, buffer, data_size - data_bytes_read);
//         if (!ret)
//         {
//             return NET_CONN_BROKE;
//         }
//         if (ret == -1)
//         {
//             return NET_ERROR;
//         }
//     } while ((data_bytes_read += ret) < data_size);

//     return data_size;
// }