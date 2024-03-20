#include <stdio.h>
#include <time.h>

extern int getInput(char* buffer, unsigned len, const char* out);
extern void print_error();
extern void printout_message(char* message, char* from, time_t timestamp);