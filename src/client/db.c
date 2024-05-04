#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sqlite3.h>

#include "queries.h"
#include "db.h"

struct DB
{
    sqlite3 *sqldb;
};

char *build_filename(const char *dir, const char *filename);

#define ENTER_TRANSACTION(db)                                         \
    if (sqlite3_exec(db, TRANSACTION, NULL, NULL, NULL) != SQLITE_OK) \
    {                                                                 \
        return HST_ERROR;                                             \
    }

#define LEAVE_TRANSACTION(db)                                    \
    if (sqlite3_exec(db, COMMIT, NULL, NULL, NULL) != SQLITE_OK) \
    {                                                            \
        return HST_ERROR;                                        \
    }

#define HANDLE_ERROR_STMT(ret, db, stmt)                             \
    if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW) \
    {                                                                \
        sqlite3_finalize(stmt);                                      \
        return HST_ERROR;                                            \
    }

#define HANDLE_ERROR_TRANSACTION(ret, db, stmt)                      \
    if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW) \
    {                                                                \
        sqlite3_exec(db, COMMIT, NULL, NULL, NULL);                  \
        sqlite3_finalize(stmt);                                      \
        return HST_ERROR;                                            \
    }

#define HANDLE_ERROR_CLOSE(ret, db) \
    if (ret != SQLITE_OK)           \
    {                               \
        sqlite3_close_v2(db);       \
        return HST_ERROR;           \
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
    int err = sqlite3_close_v2(db->sqldb);
    if (err != SQLITE_OK)
        return HST_ERROR;
    free(db);
    return HST_SUCCESS;
}

int db_get_chats(struct DB *db, chat_t **chats)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt;
    int n_chats;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, COUNT_CHATS, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    n_chats = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    chat_t *_chats = (chat_t *)malloc(sizeof(chat_t) * n_chats);
    if (!_chats)
        return HST_ERROR;

    ret = sqlite3_prepare(sqldb, PULL_CHATS, -1, &stmt, NULL);
    if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        free(_chats);
        return HST_ERROR;
    }
    int chat_idx = 0;
    while ((ret = sqlite3_step(stmt)) && chat_idx < n_chats)
    {
        if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            free(_chats);
            return HST_ERROR;
        }
        _chats[chat_idx].chat_id = sqlite3_column_int(stmt, 0);
        strncpy(_chats[chat_idx].with_user,
                (const char *)sqlite3_column_text(stmt, 1),
                USERNAME_LEN);
        chat_idx++;
    }
    sqlite3_finalize(stmt);

    if (chat_idx < n_chats)
    {
        _chats = (chat_t *)realloc(_chats, sizeof(chat_t) * chat_idx);
    }
    *chats = _chats;

    return chat_idx;
}

int db_chat_exists(struct DB *db, const int chat_id)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int exists;

    ret = sqlite3_prepare(sqldb, CHECK_CONTACT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    exists = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return exists;
}

int db_push(struct DB *db, msg_t *msg)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int ret;

    // adding chat if it's not existing
    // ret = db_chat_exists(db, user_id);
    // if (ret == HST_ERROR) return HST_ERROR;
    // if (!ret) db_add_chat(db, msg->from_id, msg->from_name);

    ret = sqlite3_prepare(sqldb, PUSH, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);

    sqlite3_bind_int(stmt, 1, msg->from_id);
    sqlite3_bind_int(stmt, 2, msg->to_id);
    sqlite3_bind_text(stmt, 3, (char *)msg->from_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, msg->timestamp);
    sqlite3_bind_int(stmt, 5, msg->text_size);
    sqlite3_bind_text(stmt, 6, msg->buffer, -1, SQLITE_STATIC);

    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_finalize(stmt);

    return HST_SUCCESS;
}

int db_count_rows(struct DB *db, const int chat_id)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int n = 0;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, COUNT_ROWS, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, chat_id);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return n;
}

int db_read_next(struct DB *db, const int chat_id, msg_t *msg, const int idx)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt = NULL;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, GET_NEXT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, chat_id);
    sqlite3_bind_int(stmt, 3, idx);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);

    if (ret == SQLITE_ROW)
    {
        // Read values from the result row
        msg->from_id = sqlite3_column_int(stmt, 1);
        msg->to_id = sqlite3_column_int(stmt, 2);
        msg->timestamp = sqlite3_column_int64(stmt, 4);
        msg->text_size = sqlite3_column_int(stmt, 5);
        strncpy(msg->buffer, (const char *)sqlite3_column_text(stmt, 6),
                MAX_MESSAGE_LEN);
    }
    else if (ret == SQLITE_DONE)
    {
        return HST_ERROR;
    }

    sqlite3_finalize(stmt);

    return HST_SUCCESS;
}

int db_add_chat(struct DB *db, const chat_t *new_chat)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, INSERT_CHAT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_bind_int(stmt, 1, new_chat->chat_id);
    sqlite3_bind_text(stmt, 2, new_chat->with_user, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return HST_SUCCESS;
}

int db_del_chat(struct DB *db, const int chat_id)
{
    sqlite3 *sqldb = db->sqldb;
    sqlite3_stmt *stmt;
    int ret;

    ret = sqlite3_prepare_v2(sqldb, DELETE_CHAT, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_bind_int(stmt, 1, chat_id);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_finalize(stmt);

    ret = sqlite3_prepare_v2(sqldb, DELETE_MESSAGES, -1, &stmt, NULL);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, chat_id);
    ret = sqlite3_step(stmt);
    HANDLE_ERROR_STMT(ret, sqldb, stmt);
    sqlite3_finalize(stmt);

    return HST_SUCCESS;
}

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