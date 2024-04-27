#include <stdbool.h>
#include "../message.h"


extern bool msg_is_valid(void* msg, const int size);
extern bool name_is_valid(username_t name);
extern bool passwd_is_valid(password_t p);

extern bool address_filter(char a);
extern bool name_filter(char a);
extern bool passwd_filter(char a);