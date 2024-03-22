
// Error codes
#define HST_SUCCESS 0
#define HST_ERROR -1
#define HST_TABLE_EMPTY -2

extern int history_push(char* dir, char* name, void* data, const unsigned size);
extern int history_pull(char* dir, char* name, void* data, const unsigned size);
extern int history_read_next(char* dir, 
                             char* name, 
                             void* data, 
                             const unsigned size, 
                             int* last_id);