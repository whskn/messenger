#include <stdbool.h>

#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 2

void logger(const int tag, const char* message, const bool printErrno);
