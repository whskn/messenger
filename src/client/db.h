#include "../message.h"
#include "interface.h"
#include "config.h"

#define EXTENSION ".chat"

// Error codes
#define HST_SUCCESS 0
#define HST_ERROR -1
#define HST_TABLE_EMPTY -2

typedef struct DB db_t;

extern int db_open(const char *dir, const char *name, struct DB **db);
extern int db_close(struct DB *db);

/* Get user's contacts */
extern int db_get_chats(struct DB *db, chat_t **chats);

/* Push a message to the db */
extern int db_push(struct DB *db, msg_t *msg);

/* Count number of messages sent from or to user_id */
extern int db_count_rows(struct DB *db, const int chat_id);

/* Adds new chat record to the database */
extern int db_add_chat(struct DB *db, const chat_t *new_chat);

/* Get idx'th last message in the chat */
extern int db_read_next(struct DB *db, const int chat_id, msg_t *msg, const int idx);

extern int db_chat_exists(struct DB *db, const int chat_id);