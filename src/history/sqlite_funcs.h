#include "sqlite/sqlite3.h"
#include "queries.h"

extern int next_id(sqlite3* db, int* id);
extern int del_row(sqlite3* db, int id);
extern int read_blob(sqlite3* db, int id, void* data, const unsigned size);
extern int get_db(char* filename, sqlite3** db);
extern int close_db(sqlite3* db);
extern int callback_id(void *data, int argc, char **argv, 
                       char **azColName __attribute__((unused)));