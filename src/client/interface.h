#ifndef _INTERFACE
#define _INTERFACE

#include <ncurses.h>
#include <time.h>

#include "../proto.h"
#include "config.h"

/* Pointer to the n'th chat in chat list */
#define CHAT(ui_data, index) (ui_data->n_of_chats > 0 &&             \
                                      index < ui_data->n_of_chats && \
                                      index >= 0                     \
                                  ? &ui_data->chats[index]           \
                                  : NULL)

/* Pointer to the current chat */
#define CURR_CHAT(ui_data) CHAT(ui_data, ui_data->curr_chat)

/* This structure is used to store chat info needed for UI work */
typedef struct
{
    int chat_id;
    username_t with_user;
} chat_t;

/* ui_t interface object. Stores all needed info for the UI. */
typedef struct
{
    char *buffer;
    int text_len;
    int buffer_size;

    msg_t **history;
    int history_len;
    int hist_head;

    chat_t *chats;
    int n_of_chats;
    int curr_chat;

    int my_id;

    WINDOW *window;
} ui_t;

/* Color pairs, defined in ui.c ui_init */
#define CLR_HEADER 1
#define CLR_FOOTER 2
#define CLR_MSG_HEADER 3
#define CLR_MY_MSG_HEADER 4
#define CLR_CHAT_SELECTOR 5
#define CLR_BADNAME 6
#define CLR_WIN_WARN 7
#define CLR_GRAYED_OUT 8
#define CLR_NORMAL 9

#endif