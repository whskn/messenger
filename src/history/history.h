extern int history_push(char* dir, char* name, void* data, const unsigned size);
extern int history_pull(char* dir, char* name, void* data, const unsigned size);
extern int history_read_next(char* dir, 
                             char* name, 
                             void* data, 
                             const unsigned size, 
                             int* last_id);