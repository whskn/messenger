#include "network.h"


extern int serv_init(conn_t** conns, sem_t** mutex, const int port);
extern int serv_get_conn(int fd, conn_t* conns, sem_t* mutex);
extern int serv_close(int fd, conn_t** conns, sem_t** mutex);

extern void* serv_manage_conn(void* args);