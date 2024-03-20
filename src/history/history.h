#include "sqlite/sqlite3.h"

extern int history_push(char* dir, char* name, char* data, const unsigned size);
extern int history_pull(char* dir, char* name, char* data, const unsigned size);
extern int history_read_next(char* dir, 
                             char* name, 
                             char* data, 
                             const unsigned size, 
                             int* last_id);