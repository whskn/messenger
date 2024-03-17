#include "history.h"
#include "../sqlite/sqlite3.h"
#include "../flags.h"

#include <string.h>
#include <stdbool.h>

#define EXTENSION ".db"

/**
 * My strnlen implementation, since the one from string.h is somehow not 
 * avaliable.
*/
size_t strnlen(const char* s, size_t len) {
    size_t i = 0;
    for (; i < len && s[i] != '\0'; ++i);
    return i;
}

int createTable(sqlite3* db) {
    char* query = "CREATE TABLE IF NOT EXISTS messages(message BLOB)";
}

int getDB(username_t name, sqlite3* db) {
    // creating filename
    unsigned nameLen = strnlen(name, sizeof(username_t));
    char filename[nameLen + sizeof(EXTENSION)];
    memcpy(filename, name, nameLen);
    strcpy(&(filename[nameLen]), EXTENSION);

    int openRet;
    if ((openRet = sqlite3_open(filename, &db)) != SQLITE_OK) {
        // error handling 
        return 1;
    }

    createTable(db);

    return 0;
}


int pushMsg(username_t name) {

}

int pullMsg(username_t name) {}