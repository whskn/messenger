#include "network.h"

extern int get_conn(int fd, conn_t* conns, sem_t* mutex);
extern void* manageConnection(void* args);