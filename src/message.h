
#ifndef _MESSAGE
#define _MESSAGE

#include <stdbool.h>
#include <sys/time.h>

// Max size of message buffer including \0
#define MAX_MESSAGE_LEN 2048
#define MAX_MESSAGE_SIZE (MAX_MESSAGE_SIZE * sizeof(char))
#define MAX_PACKET_SIZE 7000
#define USERNAME_LEN 32
#define PASSWORD_LEN 32

typedef char username_t[USERNAME_LEN];
typedef char password_t[PASSWORD_LEN];

/*
Min size of a valid msg_t structure. The real size of msg_t
structure with a two-byte message in the buffer might
be different due to paddings compiler make.
*/
#define MIN_MSG_SIZE (int)(sizeof(int) +        \
                           sizeof(int) * 2 +    \
                           sizeof(username_t) + \
                           sizeof(time_t) +     \
                           sizeof(int) +        \
                           sizeof(char) * 2)

#define msg_size(msg) (int)(msg->text_size - sizeof(msg->buffer) + sizeof(*msg))

// Communication codes
#define CC_AUTH 0x0001
#define CC_USER_RQS 0x0002
#define CC_MSG 0x0004
#define CC_PENDING_RQS 0x0010

typedef struct MsgTag
{
    int cc;

    int from_id;
    int to_id;
    username_t from_name;
    time_t timestamp;
    int text_size;
    char buffer[MAX_MESSAGE_LEN];
} msg_t;

#define AUTH_REGISTER 0x0001
#define AUTH_LOGIN 0x0002

typedef struct AuthRequestTag
{
    int cc;

    int auth_type;
    username_t username;
    password_t password;
} auth_req_t;

// HandShake codes, used to perform an authentication
#define HS_SUCC 0x0000
#define HS_MAX_CONN 0x0001
#define HS_INVAL_AUTH 0x0002
#define HS_USER_EXISTS 0x0004
#define HS_GENERIC_ERROR 0x0008
#define HS_NO_USER 0x0010
#define HS_USER_ONLINE 0x0020

typedef struct AuthResponseTag
{
    int cc;

    signed int hs_code;
    int user_id;
} auth_res_t;

typedef struct UserRequestTag
{
    int cc;

    username_t username;
} user_req_t;

typedef struct UserResponseTag
{
    int cc;

    bool exists;
    int user_id;
    username_t username;
} user_rsp_t;

#endif