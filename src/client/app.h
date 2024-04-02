#include "network.h"
#include "interface/ui.h"

extern void manageConn(connection_t* c, ui_t* ui_data, msg_t* msgin, 
                       msg_t* msgout);
extern int get_chats(username_t* chats);