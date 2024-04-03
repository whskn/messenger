#include "render.h"

#define NOTHING     0
#define MSG_TO_SEND 1 << 0
#define CHAT_SWITCH 1 << 1
#define ADD_CHAT    1 << 2

#define ui_new_message(ui_data) *ui_data->code & MSG_TO_SEND
#define ui_chat_switch(ui_data) *ui_data->code & CHAT_SWITCH
#define ui_new_chat(ui_data) *ui_data->code & ADD_CHAT

#define ME_NAMETAG "me"

#define ui_reset_code(ui_data) *ui_data->code = 0
#define ui_get_fd(ui_data) ui_data->bridge_fd
#define ui_get_buffer(ui_data) ui_data->input_buffer


// Function prototypes
extern void ui_add_chat(ui_t* ui_data, const char* username);
extern void ui_remove_chat(ui_t* ui_data, const char* username);
extern void* ui_handle(void* args);
extern void ui_close(ui_t* ui_data);
extern void ui_get_curr_chat(ui_t* ui_data, char* username);
extern ui_t* ui_init(int buffer_len, 
                     char* chats,
                     int chats_len, 
                     const int sizeof_username);

extern void ui_append_message(ui_t* ui_data, time_t timestamp, 
                              char* message, int message_size,
                              char* username);
extern void ui_clear_buffer(ui_t* ui_data);
extern void ui_get_input(ui_t* ui_data, char* data, int size, char* printout, 
                         bool (*_filter)(char));