#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "history.h"
#include "sqlite_funcs.h"


#define EXTENSION ".db"

#define GET_DB(db, dir, name, extension)                  \
    char* filename = build_filename(dir, name, EXTENSION); \
    if (filename == NULL) return HST_ERROR;                 \
    int err = get_db(filename, &db);                         \
    free(filename);                                           \
    if (err != SQLITE_OK) return HST_ERROR;

#define ENTER_TRANSACTION(db)                                          \
    if (sqlite3_exec(db, TRANSACTION, NULL, NULL, NULL) != SQLITE_OK) { \
        close_db(db);                                                    \
        return HST_ERROR;                                                 \
    }

#define LEAVE_TRANSACTION(db)                                     \
    if (sqlite3_exec(db, COMMIT, NULL, NULL, NULL) != SQLITE_OK) { \
        close_db(db);                                               \
        return HST_ERROR;                                            \
    }

#define HANDLE_ERROR_STMT(ret, stmt, db)         \
    if (ret != SQLITE_OK && ret != SQLITE_DONE) { \
        sqlite3_finalize(stmt);                    \
        sqlite3_close(db);                          \
        return HST_ERROR;                            \
    }

#define HANDLE_ERROR_TRANSACTION(ret, db)         \
    if (ret != SQLITE_OK) {                        \
        sqlite3_exec(db, COMMIT, NULL, NULL, NULL); \
        close_db(db);                                \
        return HST_ERROR;                             \
    }


/**
 * Push the data from a buffer to the database
 * 
 * @param dir directory where the db file is in, MUST END WITH '/'!. 
 *            NULL if it's in the same dir.
 * @param name of the database's file
 * @param data buffer with the data to push
 * @param size size of the data to push
 * 
 * @return error codes
*/
int history_push(char* dir, char* name, void* data, const unsigned size) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int ret;

    GET_DB(db, dir, name, EXTENSION);

    ret = sqlite3_prepare_v2(db, INSERT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, stmt, db);

    ret = sqlite3_bind_blob(stmt, 1, data, size, NULL);
    HANDLE_ERROR_STMT(ret, stmt, db);

    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, stmt, db);

    sqlite3_finalize(stmt);
    close_db(db);

    return HST_SUCCESS;
}


/**
 * Pulls the oldest entry from a db and deletes it afterwards;
 * 
 * @param name name of the database's file
 * @param data a buffer for the entry
 * @param size max size of the entry
 * 
 * @return size or readed blob or error codes, see history.h
*/
int history_pull(char* dir, char* name, void* data, const unsigned size) {
    sqlite3* db = NULL;
    int id;
    int ret;

    GET_DB(db, dir, name, EXTENSION);

    ENTER_TRANSACTION(db); // BEGIN OF AN ATOMIC OPERATION
    // Finding the row with the lowest id
    ret = sqlite3_exec(db, MIN_ID, callback_id, &id, NULL);
    if (ret == SQLITE_ABORT) {
        LEAVE_TRANSACTION(db);
        return HST_TABLE_EMPTY;
    }
    HANDLE_ERROR_TRANSACTION(ret, db);

    // reading it's blob
    int readed_size;
    ret = read_blob(db, id, data, size, &readed_size);
    HANDLE_ERROR_TRANSACTION(ret, db);

    // and then just deleting it
    ret = del_row(db, id);
    HANDLE_ERROR_TRANSACTION(ret, db);
    
    LEAVE_TRANSACTION(db); // END OF AN ATOMIC OPERATION

    close_db(db);
    return readed_size;
}

/**
 * ...
 * 
 * @param name name of the database's file
 * @param data a buffer for the entry
 * @param size max size of the entry
 * 
 * @return read blob size or negative error code
*/
int history_read_next(char* dir, 
                      char* name, 
                      void* data, 
                      const unsigned size, 
                      int* last_id) {
    sqlite3* db = NULL;
    int id = *last_id;
    int ret;

    GET_DB(db, dir, name, EXTENSION);

    ENTER_TRANSACTION(db); // BEGIN OF AN ATOMIC OPERATION
    ret = next_id(db, &id);
    HANDLE_ERROR_TRANSACTION(ret, db);

    int read_size;
    if (id != -1) {
        // reading it's blob
        ret = read_blob(db, id, data, size, &read_size);
        HANDLE_ERROR_TRANSACTION(ret, db);
    }
    LEAVE_TRANSACTION(db); // END OF AN ATOMIC OPERATION

    close_db(db);
    *last_id = id;

    return read_size;
}
