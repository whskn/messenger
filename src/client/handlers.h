#include "network.h"
#include "ui.h"


#ifndef _HANDLERS
# define _HANDLERS

extern void chat_up(ui_t* ui_data, db_t* db);
extern void chat_down(ui_t* ui_data, db_t* db);
extern void send_msg(ui_t* ui_data, connection_t* c, msg_t* msg, db_t* db);
extern void backspace(ui_t* ui_data);
extern void chat_request(ui_t* ui_data, connection_t* c);
extern void add_new_chat(ui_t* ui_data, db_t* db, void* buffer);
extern void add_char(ui_t* ui_data, char ch);
extern void incoming_msg(ui_t* ui_data, void* buffer, db_t* db, const int size);
extern void load_history(db_t* db, ui_t* ui_data);

#endif