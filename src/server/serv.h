#include <pthread.h>

#include "network.h"
#include "db.h"

typedef pthread_mutex_t mtx_t;

/* Stores info about user's connection */
typedef struct
{
    int fd;
    int user_id;
    pthread_t thread_id;

    mtx_t conn_mtx;
    db_t *db;
    void *buffer;
} conn_t;

/* Used to pass arguments to serv_manage_conn() */
typedef struct
{
    int fd;
    conn_t **page;
    mtx_t *page_mtx;
} MC_arg_t;

/* Initialize needed structures for server work (Server constructor) */
extern int serv_init(conn_t ***page, mtx_t **page_mtx, const int port);

/* Server destructor */
extern int serv_close(int fd, conn_t **page, mtx_t *page_mtx);

/* Wrapper for net_harvest_conn() in network.h */
extern int serv_get_conn(int fd);

/* Function that servs user connection */
extern void *serv_manage_conn(void *args);
