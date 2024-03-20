#include "history.h"
#include "sqlite/sqlite3.h"
#include "queries.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define EXTENSION ".db"

// funcs needed for history_push

char* build_filename(char* dir, char* filename, const char* extension);
int close_db(sqlite3* db);
int create_table(sqlite3* db);
int get_db(char* name, sqlite3** db);

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
int history_push(char* dir, char* name, char* data, const unsigned size) {
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


// funcs needed for history_pull

int callback_id(void *data, int argc, char **argv, char **azColName);
int read_blob(sqlite3* db, int id, void* data, const unsigned size);
int del_row(sqlite3* db, int id);

/**
 * Pulls the oldest entry from a db and deletes it afterwards;
 * 
 * @param name name of the database's file
 * @param data a buffer for the entry
 * @param size max size of the entry
 * 
 * @return 0 on success, sqlite3 error code otherwise.
*/
int history_pull(char* dir, char* name, char* data, const unsigned size) {
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

int next_id(sqlite3* db, int* id);

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
                      char* data, 
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
    if (ret != 0) {
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

/**
 * Replaces value form id with the next avaliable id that is > id.
 * If the value of id was negative, it writes the lowest id in the table.
 * If the table is empty, -1 is written
*/
int next_id(sqlite3* db, int* id) {
    int ret = 0;

    char* sql = sqlite3_mprintf(NEXT_ID, *id);
    if (sql == NULL) return SQLITE_ERROR;

    ret = sqlite3_exec(db, sql, callback_id, id, NULL);
    if (ret == SQLITE_ABORT) {
        *id = -1; // if id was already max
        ret = 0;
    }

    sqlite3_free(sql);
    return 0;
}

/**
 * Delete a row from db
 * 
 * @param db database itself 
 * @param id id of the row to delete
 * 
 * @return 0 - success; 
 *         sqlite error code otherwise;
 * 
*/
int del_row(sqlite3* db, int id) {
    int ret = 0;
    
    char* sql = sqlite3_mprintf(DELETE, id);
    if (sql == NULL) return SQLITE_ERROR;

    if ((ret = sqlite3_exec(db, sql, NULL, NULL, NULL)) != SQLITE_OK) {
        sqlite3_free(sql);
        return ret;
    }
    sqlite3_free(sql);

    return 0;
}

/**
 * Reads blob from a db
 * 
 * @param db pointer to a database struct
 * @param id id of the row to read blob form
 * @param data where to put readed data into
 * @param size size of data, a reading limit
 * 
 * @return 0 on success, sqlite error code otherwise.
*/
int read_blob(sqlite3* db, int id, void* data, const unsigned size) {
    sqlite3_blob* blob;
    unsigned blob_size;
    unsigned read_size;
    int ret = 0;
    
    // error handling
    if ((ret = sqlite3_blob_open(db, "main", "messages", "data", id, 0, &blob)) 
        != SQLITE_OK) {
            sqlite3_blob_close(blob);
            return ret;
        }
    blob_size = sqlite3_blob_bytes(blob);
    read_size = (blob_size < size) ? blob_size : size;
    if ((ret = sqlite3_blob_read(blob, data, read_size, 0)) != SQLITE_OK) {
        sqlite3_blob_close(blob);
        return ret;
    }
    sqlite3_blob_close(blob);

    return 0;
}

/**
 * Callback function for getting min id request
 * 
 * @return 0 if min ID was saved to data, 1 if not;
*/
int callback_id(void *data, 
                    int argc, 
                    char **argv, 
                    char **azColName __attribute__((unused))){
    if (argc < 1 || argv[0] == NULL) return 1;

    int *id = (int *)data;
    *id = atoi(argv[0]);
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
 * Open or create database with following name
 * 
 * @param name name of the database file without an extension
 * @param db where to store a pointer to the db
 * 
 * @return 0 on success, sqlite error code otherwise;
*/
int get_db(char* filename, sqlite3** db) {
    int ret = 0;

    if ((ret = sqlite3_open(filename, db)) != SQLITE_OK) {
        close_db(*db);
        return ret;
    }

    if ((ret = create_table(*db)) != 0) {
        close_db(*db);
        return ret;
    }

    return 0;
}

/**
 * Creates table in the database (if not exists)
 * 
 * @param db database where a table needs to be created 
 * @return 0 on success, sqlite error code otherwise.
*/
int create_table(sqlite3* db) {
    int ret = 0;
    if ((ret = sqlite3_exec(db, CREATE_TABLE, NULL, NULL, NULL)) != SQLITE_OK) {
        return ret;
    }
    return 0;
}

/**
 * Closes database
 * 
 * @param db opened database itself
 * @return 0 - success, sqlite error code otherwise.
*/
int close_db(sqlite3* db) {
    int ret = 0;
    if ((ret = sqlite3_close_v2(db)) != SQLITE_OK) return ret;
    return 0;
}