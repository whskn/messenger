#include "interface.h"

#define ui_get_buffer(ui_data) ui_data->buffer


// Function prototypes
extern ui_t* ui_init(chat_t* chats, const int n_of_chats);
extern void ui_close(ui_t* ui_data);

extern void ui_render_window(ui_t* ui_data);
extern void ui_add_chat(ui_t* ui_data, chat_t* chat);
extern void ui_remove_chat(ui_t* ui_data, chat_t* chat);
extern void ui_append_message(ui_t* ui_data, msg_t* msg);
extern void ui_clear_buffer(ui_t* ui_data);
extern void ui_get_input(ui_t* ui_data, char* data, int size, char* printout, 
                         bool (*_filter)(char), const bool hide_chars);
extern void ui_clear_history(ui_t* ui_data);
extern void ui_curr_chat_name(ui_t* ui_data, username_t username);
extern void ui_switch_chat(ui_t* ui_data, const int step);
extern void ui_set_my_id(ui_t* ui_data, const int my_id);
extern void ui_warning(ui_t* ui_data, const char* text);
extern void ui_flush_stdin();
extern char* loading_ani();