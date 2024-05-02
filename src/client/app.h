#include "network.h"
#include "ui.h"
#include "db.h"

/**
 * Detects events on both UI and server-connection fd and calls handlers
 */
extern void manage_conn(connection_t *c, ui_t *ui_data, void *buffer, db_t *db);