#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define CLR_GOOD "\033[0;32m"
#define CLR_ERROR "\033[0;31m"
#define CLR_WARNING "\033[0;33m"
#define CLR_INFO "\033[0m"

/** Print out log message
 * 
 * @param tag LOG_ERROR, LOG_WARNING, LOG_INFO
 * @param message message to output
 * @param printErrno output errno message?
*/
void logger(const int tag, const char* message, const bool printErrno) {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char strTime[28];
    sprintf(strTime, "%02d.%02d.%02d %02d:%02d:%02d",
            timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year % 100,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    char* color;
    char* resetClr = "\033[0m";
    char* badge;
    
    if (tag == LOG_ERROR) {
        color = CLR_ERROR;
        badge = "ERROR";
    } 
    else if (tag == LOG_WARNING) {
        color = CLR_WARNING;
        badge = "WARN";
    }
    else if (tag == LOG_GOOD) {
        color = CLR_GOOD;
        badge = "GOOD";
    }
    else {
        color = CLR_INFO;
        badge = "INFO";
    }

    if (printErrno) {
        printf("%s %s[%s]:%s %s (%s)\n", 
                strTime, 
                color,
                badge,
                resetClr,
                message,
                strerror(errno));
    } else {
        printf("%s %s[%s]:%s %s\n", 
                strTime, 
                color,
                badge,
                resetClr,
                message);
    }
}
