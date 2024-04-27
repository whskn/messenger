#include <semaphore.h>

#include "network.h"
#include "db.h"

typedef struct {
    int fd;
    int user_id;
    
    db_t* db;
    void* buffer;
} conn_t;

typedef struct {
    int fd;
    conn_t** conns;
    sem_t* mutex;
} MC_arg_t;

extern int serv_init(conn_t*** conns, sem_t** mutex, const int port);
extern int serv_get_conn(int fd);
extern int serv_close(int fd, conn_t** conns, sem_t** mutex);
extern void* serv_manage_conn(void* args);
