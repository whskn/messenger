#include <pthread.h>

#include "network.h"
#include "db.h"

typedef pthread_mutex_t mtx_t;

typedef struct {
    int fd;
    int user_id;
    pthread_t thread_id;
    
    mtx_t conn_mtx;
    db_t* db;
    void* buffer;
} conn_t;

typedef struct {
    int fd;
    conn_t** conns;
    mtx_t* page_mtx;
} MC_arg_t;

extern int serv_init(conn_t*** conns, mtx_t** page_mtx, const int port);
extern int serv_get_conn(int fd);
extern int serv_close(int fd, conn_t** conns, mtx_t* page_mtx);
extern void* serv_manage_conn(void* args);
