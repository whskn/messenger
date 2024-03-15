#include <stdio.h>

typedef char username_t[32];

extern int get_username(username_t buffer, const char* out);
extern void print_error();