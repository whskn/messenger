/* This header file provides event handlers.

These functions are intercept-point of db.h, network.h and ui.h,
the idea was to put everything together for handy interaction.
After you get an event from stdin or server-connection fd, all is needed is to
identify it and call one of these functions to handle the event.
*/

#ifndef _HANDLERS
#define _HANDLERS

#include "network.h"
#include "ui.h"

extern void h_chat_up(ui_t *ui_data, db_t *db);
extern void h_chat_down(ui_t *ui_data, db_t *db);
extern void h_chat_request(ui_t *ui_data, connection_t *c);
extern void h_add_new_chat(ui_t *ui_data, db_t *db, void *buffer);

extern void h_send_msg(ui_t *ui_data, connection_t *c, msg_t *msg, db_t *db);
extern void h_incoming_msg(ui_t *ui_data, void *buffer, db_t *db, const int size);

extern void h_add_char(ui_t *ui_data, char ch);
extern void h_backspace(ui_t *ui_data);

extern void h_load_history(db_t *db, ui_t *ui_data);
extern void h_del_chat(ui_t *ui_data, db_t *db);

#endif