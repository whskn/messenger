#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sqlite3.h>

#include "logger.h"
#include "queries.h"
#include "db.h"

struct DB
{
    sqlite3 *sqldb;
};

#define ENTER_T(sqldb, stmt)                                             \
    if (sqlite3_exec(sqldb, TRANSACTION, NULL, NULL, NULL) != SQLITE_OK) \
    {                                                                    \
        sqlite3_finalize(stmt);                                          \
        return HST_ERROR;                                                \
    }

#define LEAVE_T(sqldb)                                              \
    if (sqlite3_exec(sqldb, COMMIT, NULL, NULL, NULL) != SQLITE_OK) \
    {                                                               \
        return HST_ERROR;                                           \
    }

char *build_filename(const char *dir, const char *filename);

#define HANDLE_ERROR_STMT(ret, stmt)                                 \
    if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW) \
    {                                                                \
        sqlite3_finalize(stmt);                                      \
        return HST_ERROR;                                            \
    }

#define HANDLE_ERROR_CLOSE(ret, sqldb) \
    if (ret != SQLITE_OK)              \
    {                                  \
        sqlite3_close_v2(sqldb);       \
        return HST_ERROR;              \
    }

int db_open(const char *dir, const char *name, struct DB **db)
{
    int ret;
    struct DB *temp_db;
    sqlite3 *sqldb = NULL;

    char *filename = build_filename(dir, name);
    if (filename == NULL)
        return HST_ERROR;

    ret = sqlite3_open(filename, &sqldb);
    free(filename);
    HANDLE_ERROR_CLOSE(ret, sqldb);

    ret = sqlite3_exec(sqldb, CREATE_CLI_TABLES, NULL, NULL, NULL);
    HANDLE_ERROR_CLOSE(ret, sqldb);

    temp_db = (struct DB *)malloc(sizeof(struct DB));
    temp_db->sqldb = sqldb;
    *db = temp_db;

    return HST_SUCCESS;
}

int db_close(struct DB *db)
{
    if (db == NULL)
        return HST_SUCCESS;
    int err = sqlite3_close_v2(db->sqldb);
    if (err != SQLITE_OK)
        return HST_ERROR;
    free(db);
    return HST_SUCCESS;
}

int db_push(struct DB *db, msg_t *msg, const bool sent)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt;
    int ret;
    int message_id;

    ret = sqlite3_prepare_v2(sqldb, NEW_MSG, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, stmt);

    sqlite3_bind_int(stmt, 1, msg->from_id);
    sqlite3_bind_int(stmt, 2, msg->to_id);
    sqlite3_bind_int(stmt, 3, msg->timestamp);
    sqlite3_bind_int(stmt, 4, msg->text_size);
    sqlite3_bind_text(stmt, 5, msg->buffer, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, (int)sent);

    ENTER_T(sqldb, stmt);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE)
    {
        LEAVE_T(sqldb);
        sqlite3_finalize(stmt);
        return HST_ERROR;
    }
    message_id = sqlite3_last_insert_rowid(sqldb);
    sqlite3_finalize(stmt);
    LEAVE_T(sqldb);

    return message_id;
}

int db_next_unsent(struct DB *db, msg_t *msg, const int to_id,
                   unsigned long *timestamp)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int new_ts = 0;
    int message_id = HST_TABLE_EMPTY;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, PULL_UNSENT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, stmt);
    sqlite3_bind_int(stmt, 1, to_id);
    sqlite3_bind_int(stmt, 2, *timestamp);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, stmt);

    if (ret == SQLITE_ROW)
    {
        message_id = sqlite3_column_int(stmt, 0);
        msg->to_id = sqlite3_column_int(stmt, 1);
        msg->from_id = sqlite3_column_int(stmt, 2);
        msg->timestamp = sqlite3_column_int(stmt, 3);
        msg->text_size = sqlite3_column_int(stmt, 4);
        strncpy(msg->buffer,
                (const char *)sqlite3_column_text(stmt, 5),
                MAX_MESSAGE_LEN);
        strncpy(msg->from_name,
                (const char *)sqlite3_column_text(stmt, 6),
                USERNAME_LEN);
        new_ts = sqlite3_column_int(stmt, 3);
    }

    sqlite3_finalize(stmt);
    *timestamp = new_ts;

    return message_id;
}

int db_mark_as_sent(struct DB *db, const int message_id)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, MARK_AS_SENT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, stmt);
    sqlite3_bind_int(stmt, 1, message_id);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, stmt);

    sqlite3_finalize(stmt);
    return HST_SUCCESS;
}

int db_new_user(struct DB *db, username_t name, password_t password,
                user_t *user)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int user_id;
    const char *err;

    ret = sqlite3_prepare_v2(sqldb, NEW_USER, -1, &stmt, &err);
    if (ret != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return HST_ERROR;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    ENTER_T(sqldb, stmt);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (ret == SQLITE_CONSTRAINT)
    {
        LEAVE_T(sqldb);
        return HST_CONSTRAINT;
    }
    if (ret != SQLITE_DONE)
    {
        LEAVE_T(sqldb);
        return HST_ERROR;
    }
    user_id = sqlite3_last_insert_rowid(sqldb);
    LEAVE_T(sqldb);

    user->user_id = user_id;
    strncpy(user->username, name, USERNAME_LEN);
    strncpy(user->password, password, PASSWORD_LEN);

    return user_id ? HST_SUCCESS : HST_ERROR;
}

int db_get_user(struct DB *db, username_t name, user_t *user)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    char *password_ptr, *username_ptr;
    int ret;
    const char *err;

    ret = sqlite3_prepare_v2(sqldb, GET_USER, -1, &stmt, &err);
    HANDLE_ERROR_STMT(ret, stmt);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, stmt);

    if (ret == SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return HST_NOUSER;
    }

    user->user_id = sqlite3_column_int(stmt, 0);
    if ((password_ptr = (char *)sqlite3_column_text(stmt, 2)) == NULL ||
        (username_ptr = (char *)sqlite3_column_text(stmt, 1)) == NULL)
    {
        sqlite3_finalize(stmt);
        return HST_ERROR;
    }
    memcpy(user->username, username_ptr, sizeof(password_t));
    memcpy(user->password, password_ptr, sizeof(password_t));

    sqlite3_finalize(stmt);
    return HST_SUCCESS;
}

/**
 * This function concatinates dir, filename and extension strings and
 * returns a pointer to the string. Returns NULL if allocation failed.
 *
 * Calling thread must free() memory.
 */
char *build_filename(const char *dir, const char *filename)
{
    const int max_dir_size = 4096;
    const int max_filename_size = 256;

    int size = (dir != NULL ? strnlen(dir, max_dir_size) : 0) +
               strnlen(filename, max_filename_size) + 1; // +1 for \0 byte

    char *buffer = (char *)malloc(size);
    if (buffer == NULL)
    {
        return NULL;
    }

    buffer[0] = '\0'; // so strcat will work like strcpy

    if (dir != NULL)
    {
        strncat(buffer, dir, max_dir_size);
    }
    strncat(buffer, filename, max_filename_size);

    return buffer;
}