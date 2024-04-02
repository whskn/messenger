#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite_funcs.h"

/**
 * Replaces value form id with the next avaliable id that is > current id.
 * 
 * If the value was negative, it writes the lowest id in the table down.
 * If the table is empty, -1 is written.
 * If there is no id > current id, -1 is written.
 * 
 * @return sqlite3 error codes
*/
int next_id(sqlite3* db, int* id) {
    int ret = SQLITE_OK;

    char* sql = sqlite3_mprintf(NEXT_ID, *id);
    if (sql == NULL) {
        return SQLITE_NOMEM;
    }

    ret = sqlite3_exec(db, sql, callback_id, id, NULL);
    if (ret == SQLITE_ABORT) {
        *id = -1; // if id was already max
        ret = SQLITE_OK;
    }

    sqlite3_free(sql);
    return ret;
}

/**
 * Delete a row from db
 * 
 * @param db database itself 
 * @param id id of the row to delete
 * 
 * @return sqlite error codes
 * 
*/
int del_row(sqlite3* db, int id) {
    int ret = SQLITE_OK;
    
    char* sql = sqlite3_mprintf(DELETE, id);
    if (sql == NULL) {
        return SQLITE_NOMEM;
    }

    ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
    sqlite3_free(sql);

    return ret;
}


/**
 * Reads blob from a db
 * 
 * @param db pointer to a database struct
 * @param id id of the row to read blob form
 * @param data where to put readed data into
 * @param size size of data, a reading limit
 * @param save_size_to where to put size of readed blob, or just NULL
 * 
 * @return sqlite error code
*/
int read_blob(sqlite3* db, int id, void* data, const int size, 
              int* save_size_to) {
    sqlite3_blob* blob;
    int blob_size;
    int read_size;
    int ret = SQLITE_OK;
    
    // error handling
    ret = sqlite3_blob_open(db, "main", "messages", "data", id, 0, &blob);
    if (ret != SQLITE_OK) {
        sqlite3_blob_close(blob);
        return ret;
    }

    blob_size = sqlite3_blob_bytes(blob);
    read_size = (blob_size < size) ? blob_size : size;
    ret = sqlite3_blob_read(blob, data, read_size, 0);

    if (save_size_to != NULL) {
        *save_size_to = read_size;
    }

    sqlite3_blob_close(blob);
    return ret;
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
 * Creates table in the database (if not exists)
 * 
 * @param db database where a table needs to be created 
 * 
 * @return sqlite error codes
*/
int create_table(sqlite3* db) {
    return sqlite3_exec(db, CREATE_TABLE, NULL, NULL, NULL);
}

/**
 * Open or create database with following name
 * 
 * @param name name of the database file without an extension
 * @param db where to store a pointer to the db
 * 
 * @return sqlite error code otherwise;
*/
int get_db(char* filename, sqlite3** db) {
    int ret;

    ret = sqlite3_open(filename, db);
    if (ret != SQLITE_OK) {
        close_db(*db);
        return ret;
    }

    ret = create_table(*db);
    if (ret != SQLITE_OK) {
        close_db(*db);
    }

    return ret;
}

/**
 * Closes database
 * 
 * @param db opened database itself
 * @return sqlite error code otherwise.
*/
int close_db(sqlite3* db) {
    return sqlite3_close_v2(db);
}


/**
 * This function concatinates dir, filename and extension strings and 
 * returns a pointer to the string. Returns NULL if allocation failed.
 * 
 * Calling thread must free() memory.
*/
char* build_filename(char* dir, char* filename, const char* extension) {
    int size = (dir != NULL) ? strlen(dir) : 0 + 
                    strlen(filename) + 
                    strlen(extension) + 1; // +1 for \0 byte 

    char* buffer = (char*)malloc(size);
    if (buffer == NULL) {
        return NULL;
    }

    buffer[0] = '\0'; // so strcat will work like strcpy

    if (dir != NULL) {
        strcat(buffer, dir);
    }
    strcat(buffer, filename);
    strcat(buffer, extension);

    return buffer;
}