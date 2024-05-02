/* This header file provides interface to interact with UI */

#include "interface.h"

#define ui_get_buffer(ui_data) ui_data->buffer

/* Constructor for ui_t */
extern ui_t *ui_init(chat_t *chats, const int n_of_chats);

/* Destructor for ui_t */
extern void ui_close(ui_t *ui_data);

/* Renders the main interface window with chats, message input field etc. */
extern void ui_render_window(ui_t *ui_data);

/* Adds a new chat to UI buffer, so it will be displayed in the chat list
after the next call of ui_render_window */
extern void ui_add_chat(ui_t *ui_data, chat_t *chat);

/* Removes chat from UI chat list buffer. The chat will not be shown after the
next call of ui_render_window */
extern void ui_remove_chat(ui_t *ui_data, chat_t *chat);

/* Appends message to the currently shown message list */
extern void ui_append_message(ui_t *ui_data, msg_t *msg);

/* Clears message input buffer */
extern void ui_clear_buffer(ui_t *ui_data);

/* Blocks runtime until user inputs some text. An input field is shown.*/
extern void ui_get_input(ui_t *ui_data, char *data, int size, char *printout,
                         bool (*filter)(const int));

/* Clears chat history */
extern void ui_clear_history(ui_t *ui_data);

/* Switches current chat */
extern void ui_switch_chat(ui_t *ui_data, const int step);

/* Gets user's id. When chat is rendered username is replaced by @me
based on this id.*/
extern void ui_set_my_id(ui_t *ui_data, const int my_id);

/* Renders warning page */
extern void ui_warning(ui_t *ui_data, const char *text);

/* Flushes everything what's in stdin */
extern void ui_flush_stdin();

/* Blocks runtime until user enters his username and password.
filter_0 and filter_1 are functions that get char as input and return bool
based on if the char is legal in the input. */
extern int ui_login(ui_t *ui_data, username_t *username, password_t *password,
                    bool (*filter_0)(const int), bool (*filter_1)(const int));

extern char *loading_ani();