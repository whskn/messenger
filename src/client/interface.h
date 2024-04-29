#ifndef _INTERFACE
# define _INTERFACE

#include <ncurses.h>
#include <time.h>
#include "../message.h"
#include "config.h"

#define ME_NAMETAG "me"

#define CHAT(ui_data, index) (ui_data->n_of_chats > 0 && \
                              index < ui_data->n_of_chats  && \
                              index >= 0 \
                              ? &ui_data->chats[index] \
                              : NULL)
#define CURR_CHAT(ui_data) CHAT(ui_data, ui_data->curr_chat)

typedef struct {
    int chat_id;
    username_t with_user;
} chat_t;

typedef struct {
    char* buffer;
    int text_len;
    int buffer_size;

    msg_t** history;
    int history_len;
    int hist_head;

    chat_t* chats;
    int n_of_chats;
    int curr_chat;

    int my_id;

    WINDOW* window;
} ui_t;

#define CLR_HEADER          1
#define CLR_FOOTER          2
#define CLR_MSG_HEADER      3
#define CLR_MY_MSG_HEADER   4
#define CLR_CHAT_SELECTOR   5
#define CLR_BADNAME         6
#define CLR_WIN_WARN        7
#define CLR_GRAYED_OUT      8

#endif