
extern int blocking_read(int fd, void* buffer, const int size, const int timeout);

// Error codes
#define BR_CHECK_ERRNO -1
#define BR_TIMEOUT -2
#define BR_EOF -4