#include "../proto.h"

// Error codes
#define HST_SUCCESS 0
#define HST_ERROR -1
#define HST_TABLE_EMPTY -2
#define HST_CONSTRAINT -3
#define HST_NOUSER -4

typedef struct DB db_t;

typedef struct UserTag
{
    int user_id;
    username_t username;
    password_t password;
} user_t;

extern int db_open(const char *dir, const char *filename, struct DB **db);
extern int db_close(struct DB *db);

/* Push message to database */
extern int db_push(struct DB *db, msg_t *msg, const bool sent);

/* Get next unsent message with timestamp greather than the argument */
extern int db_next_unsent(struct DB *db, msg_t *msg, const int to_id,
                          unsigned long *timestamp);

/* Add new user to the db */
extern int db_new_user(struct DB *db, username_t name, password_t password,
                       user_t *user);

/* Get user from the db */
extern int db_get_user(struct DB *db, username_t name, user_t *user);

/* Mark message as sent */
extern int db_mark_as_sent(struct DB *db, const int message_id);