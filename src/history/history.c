#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "history.h"
#include "sqlite_funcs.h"


#define EXTENSION ".db"


/**
 * Push the data from a buffer to the database
 * 
 * @param dir directory where the db file is in, MUST END WITH '/'!. 
 *            NULL if it's in the same dir.
 * @param name of the database's file
 * @param data buffer with the data to push
 * @param size size of the data to push
 * 
 * @return 0 on success, sqlite3 error code otherwise;
*/
int history_push(char* dir, char* name, void* data, const unsigned size) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int ret = 0;

    char* filename = build_filename(dir, name, EXTENSION);
    if (filename == NULL) return 1;

    if ((ret = get_db(filename, &db)) != 0) {
        free(filename);
        return ret;
    } 
    free(filename);

    if ((ret = sqlite3_prepare_v2(db, INSERT, -1, &stmt, NULL)) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        close_db(db);
        return ret;
    }
    if ((ret = sqlite3_bind_blob(stmt, 1, data, size, NULL)) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        close_db(db);
        return ret;
    }
    if ((ret = sqlite3_step(stmt)) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        close_db(db);
        return ret;
    }

    sqlite3_finalize(stmt);
    if ((ret = close_db(db)) != 0) return ret;

    return 0;
}


/**
 * Pulls the oldest entry from a db and deletes it afterwards;
 * 
 * @param name name of the database's file
 * @param data a buffer for the entry
 * @param size max size of the entry
 * 
 * @return 0 on success, sqlite3 error code otherwise.
*/
int history_pull(char* dir, char* name, void* data, const unsigned size) {
    sqlite3* db = NULL;
    int id;
    int ret;

    char* filename = build_filename(dir, name, EXTENSION);
    if (filename == NULL) return 1;

    if ((ret = get_db(filename, &db)) != 0) {
        free(filename);
        return ret;
    } 
    free(filename);

    // Begin of the atomic operation -------------
    if ((ret = sqlite3_exec(db, TRANSACTION, NULL, NULL, NULL)) != SQLITE_OK) {
            close_db(db);
            return ret;
        }

    // Finding the row with the lowest id
    ret = sqlite3_exec(db, MIN_ID, callback_id, &id, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
        close_db(db);
        return ret;
    }

    // reading it's blob
    ret = read_blob(db, id, data, size);
    if (ret != 0) {
        sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
        close_db(db);
        return ret;
    }
    
    // and then just deleting it
    ret = del_row(db, id);
    if (ret != 0) {
        sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
        close_db(db);
        return ret;
    }
    
    ret = sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        close_db(db);
        return ret;
    }
    // End of the atomic operation --------------

    if ((ret = close_db(db)) != 0) return ret;
    return 0;
}

/**
 * This function concatinates dir, filename and extension strings and 
 * returns a pointer to the string. Returns NULL if allocation failed.
 * 
 * Calling thread must free() memory.
*/
char* build_filename(char* dir, char* filename, const char* extension) {
    unsigned size = (dir != NULL) ? strlen(dir) : 0 + 
                    strlen(filename) + 
                    strlen(extension) + 1; // +1 for \0 byte 

    char* buffer = (char*)malloc(size);
    if (buffer == NULL) return NULL;

    buffer[0] = '\0'; // so strcat will work like strcpy

    if (dir != NULL) strcat(buffer, dir);
    strcat(buffer, filename);
    strcat(buffer, extension);

    return buffer;
}

/**
 * ...
 * 
 * @param name name of the database's file
 * @param data a buffer for the entry
 * @param size max size of the entry
 * 
 * @return 0 on success, sqlite3 error code otherwise.
*/
int history_read_next(char* dir, 
                      char* name, 
                      void* data, 
                      const unsigned size, 
                      int* last_id) {
    sqlite3* db = NULL;
    int id = *last_id;
    int ret;

    char* filename = build_filename(dir, name, EXTENSION);
    if (filename == NULL) return 1;

    if ((ret = get_db(filename, &db)) != 0) {
        free(filename);
        return ret;
    }
    free(filename);

    // Begin of the atomic operation -------------
    if ((ret = sqlite3_exec(db, TRANSACTION, NULL, NULL, NULL)) != SQLITE_OK) {
            close_db(db);
            return ret;
        }

    ret = next_id(db, &id);
    if (ret != SQLITE_OK) {
        sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
        close_db(db);
        return ret;
    }
    if (id != -1) {
        // reading it's blob
        ret = read_blob(db, id, data, size);
        if (ret != 0) {
            sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
            close_db(db);
            return ret;
        }
    }
    
    ret = sqlite3_exec(db, COMMIT, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        close_db(db);
        return ret;
    }
    // End of the atomic operation --------------

    if ((ret = close_db(db)) != 0) return ret;
    *last_id = id;

    return 0;
}
