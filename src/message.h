#include <time.h>

#define MAX_MESSAGE_LENGTH 2048

// sizeof(username_t) * 2 + sizeof(time_t) + sizeof(size_t) + sizeof(char) = 81
#define MIN_MESSAGE_LEN 81

typedef char username_t[32];

typedef struct {
    username_t from;
    username_t to;
} fromto_t;

typedef struct {
    fromto_t names;
    time_t timestamp;
    size_t msg_size;
    char buffer[MAX_MESSAGE_LENGTH];
} msg_t;