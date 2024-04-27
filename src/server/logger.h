#include <stdbool.h>

#define LOG_GOOD 0
#define LOG_INFO 1
#define LOG_WARNING 2
#define LOG_ERROR 3

extern void logger(const int tag, const char* message, const bool printErrno);
