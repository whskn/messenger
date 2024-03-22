#include <time.h>

// Max size of message buffer including \0
#define MAX_MESSAGE_SIZE 2048

/*
Min size of a valid msg_t structure. The real size of msg_t 
structure with a two-byte message in the buffer might 
be different due to paddings compiler make. 
*/
#define MIN_MSG_SIZE (int)(sizeof(username_t) * 2 + \
                           sizeof(time_t) + \
                           sizeof(int) + \
                           sizeof(char) * 2)


typedef char username_t[32];

typedef struct {
    username_t from;
    username_t to;
} fromto_t;

typedef struct {
    fromto_t names;
    time_t timestamp;
    int text_size;
    char buffer[MAX_MESSAGE_SIZE];
} msg_t;