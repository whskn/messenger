#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "render.h"
#include "ui.h"
#include "ui_config.h"
#include "../misc/validate.h"

ui_t *ui_init(chat_t *chats, const int n_of_chats)
{

    // init screen
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // build the struct
    ui_t *ui_data = (ui_t *)malloc(sizeof(ui_t));

    ui_data->buffer = (char *)malloc(sizeof(char) * MAX_MESSAGE_LEN);
    ui_data->buffer_size = MAX_MESSAGE_LEN;
    ui_data->text_len = 0;
    ui_data->history = (msg_t **)calloc(sizeof(msg_t *), MAX_HIST);
    ui_data->history_len = MAX_HIST;
    ui_data->hist_head = 0;
    ui_data->chats = chats;
    ui_data->n_of_chats = n_of_chats;
    ui_data->curr_chat = 0;
    ui_data->window = stdscr;

    // init colors
    start_color();
    init_pair(CLR_HEADER, FONT_HEADER, BG_HEADER);
    init_pair(CLR_FOOTER, FONT_FOOTER, BG_FOOTER);
    init_pair(CLR_MSG_HEADER, FONT_MSG_HEADER, BG_MSG_HEADER);
    init_pair(CLR_MY_MSG_HEADER, FONT_MY_MSG_HEAER, BG_MY_MSG_HEADER);
    init_pair(CLR_CHAT_SELECTOR, FONT_CHAT_SELECTOR, BG_CHAT_SELECTOR);
    init_pair(CLR_BADNAME, FONT_BADNAME, BG_BADNAME);
    init_pair(CLR_WIN_WARN, FONT_WIN_WARN, BG_WIN_WARN);
    init_pair(CLR_GRAYED_OUT, FONT_GRAYED_OUT, BG_GRAYED_OUT);

    return ui_data;
}

static bool check_win_size(ui_t *ui_data)
{
    int width = getmaxx(ui_data->window);
    int height = getmaxy(ui_data->window);

    if (width < MIN_WIN_WIDTH || height < MIN_WIN_HEIGHT)
    {
        render_size_warning(ui_data);
        return false;
    }
    return true;
}

void ui_switch_chat(ui_t *ui_data, const int step)
{
    int new_chat_idx = ui_data->curr_chat + step;

    if (new_chat_idx >= ui_data->n_of_chats)
    {
        ui_data->curr_chat = ui_data->n_of_chats - 1;
    }
    else if (new_chat_idx < 0)
    {
        ui_data->curr_chat = 0;
    }
    else
    {
        ui_data->curr_chat = new_chat_idx;
    }
}

void ui_set_my_id(ui_t *ui_data, const int my_id)
{
    ui_data->my_id = my_id;
}

void ui_render_window(ui_t *ui_data)
{
    if (check_win_size(ui_data))
    {
        clear();
        render_top_bar(ui_data);
        render_side_bar(ui_data);
        render_msg_hist(ui_data);
        render_footer(ui_data);
        render_msg_input(ui_data);
    }
    refresh();
}

bool default_filter(const int a __attribute__((unused)))
{
    return true;
}

int ui_login(ui_t *ui_data,
             username_t *username,
             password_t *password,
             bool (*filter_0)(const int),
             bool (*filter_1)(const int))
{
    char *buff_0 = (char *)calloc(1, sizeof(username_t));
    char *buff_1 = (char *)calloc(1, sizeof(password_t));

    int len_0 = 0;
    int len_1 = 0;

    int button = 0;
    int field = 0;

    while (true)
    {
        if (!check_win_size(ui_data))
        {
            while (getch() != KEY_RESIZE)
                ;
            continue;
        }

        clear();
        render_login_win(ui_data, buff_0, len_0, buff_1, len_1, button, field);

        int ch = getch();
        switch (ch)
        {
        case '\n':
            memcpy(username, buff_0, sizeof(username_t));
            memcpy(password, buff_1, sizeof(password_t));
            free(buff_0);
            free(buff_1);
            return button;

        case KEY_RESIZE:
            break;

        case KEY_BACKSPACE:
            if (field == 0 && len_0)
                buff_0[--len_0] = '\0';
            else if (field == 1 && len_1)
                buff_1[--len_1] = '\0';
            break;

        case KEY_UP:
            field = 0;
            break;

        case KEY_DOWN:
            field = 1;
            break;

        case KEY_LEFT:
            button = 0;
            break;

        case KEY_RIGHT:
            button = 1;
            break;

        default:
            if (field == 0 && len_0 < USERNAME_LEN - 1 && filter_0(ch))
            {
                buff_0[len_0++] = (char)ch;
                buff_0[len_0] = '\0';
            }
            else if (field == 1 && len_1 < PASSWORD_LEN - 1 && filter_1(ch))
            {
                buff_1[len_1++] = (char)ch;
                buff_1[len_1] = '\0';
            }
            break;
        }
    }

    free(buff_0);
    free(buff_1);
    return button;
}

void ui_get_input(ui_t *ui_data, char *data, int size, char *printout,
                  bool (*_filter)(const int))
{
    // TEHDOLG hide chars when variable is true
    // TEHDOLG check name after ENTER is pressed (empty names somehow aren't detected)
    // TEHDOLG name_filter, passwd_filter doesn't work (they pass unwanted chars)
    char *temp_buff = (char *)malloc(size * sizeof(char));
    temp_buff[0] = '\0';
    int printout_len = strlen(printout);
    int len = 0;
    bool badname = false;
    bool (*filter)(const int) = _filter ? _filter : default_filter;

    while (true)
    {
        if (!check_win_size(ui_data))
        {
            while (getch() != KEY_RESIZE)
                ;
            continue;
        }

        clear();
        render_get_input(ui_data, printout, printout_len, temp_buff, len,
                         badname);

        int ch = getch();
        badname = false;
        switch (ch)
        {
        case '\n':
            for (int i = 0; i < len - 1; i++)
            {
                if (!filter(temp_buff[i]))
                {
                    badname = true;
                    break;
                }
            }
            if (badname)
                break;

            memcpy(data, temp_buff, size);
            free(temp_buff);
            return;

        case KEY_RESIZE:
            break;

        case KEY_BACKSPACE:
            if (len > 0)
                temp_buff[--len] = '\0';
            break;

        default:
            if (len < size - 1)
            { // for new char and \0
                temp_buff[len++] = (char)ch;
                temp_buff[len] = '\0';
            }
            break;
        }
    }
    ui_clear_buffer(ui_data);
}

void ui_flush_stdin()
{
    const int size = 32;
    char *buffer[size];
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    while (read(STDIN_FILENO, buffer, size) > 0)
        ;
    fcntl(STDIN_FILENO, F_SETFL, flags);
}

void ui_warning(ui_t *ui_data, const char *text)
{
    render_warning(ui_data, text);
    move(0, 0);
    refresh();
}

char *loading_ani()
{
    static char *placeholder = NULL;
    static int dots = 0;
    const int text_len = sizeof(CONNECTING) - 1;

    if (!placeholder)
    {
        placeholder = (char *)malloc(text_len + MAX_DOTS + 1); // +1 for '\0'
        strcpy(placeholder, CONNECTING);
    }

    for (int i = 0; i < dots; i++)
    {
        placeholder[text_len + i] = '.';
    }
    placeholder[text_len + dots] = '\0';

    dots = ++dots <= 3 ? dots : 0;
    return placeholder;
}

void ui_add_chat(ui_t *ui_data, chat_t *chat)
{
    int idx = 0;
    for (; idx < ui_data->n_of_chats; idx++)
    {
        if (chat->chat_id == ui_data->chats[idx].chat_id)
            break;
    }
    if (idx < ui_data->n_of_chats)
    {
        ui_data->curr_chat = idx;
        return;
    }

    ui_data->chats = (chat_t *)realloc(ui_data->chats,
                                       ++ui_data->n_of_chats * sizeof(chat_t));

    chat_t *last_cell = &ui_data->chats[ui_data->n_of_chats - 1];
    memcpy(last_cell, chat, sizeof(chat_t));
    ui_data->curr_chat = ui_data->n_of_chats - 1;
}

void ui_remove_chat(ui_t *ui_data, chat_t *chat)
{
    // TEHDOLG
    int idx = 0;
    for (; idx < ui_data->n_of_chats; idx++)
    {
        if (chat->chat_id == ui_data->chats[idx].chat_id)
            break;
    }
    if (idx == ui_data->n_of_chats)
        return;

    // shift further names
    chat_t *dst, *src;
    for (int i = idx + 1; i < ui_data->n_of_chats; i++)
    {
        dst = &ui_data->chats[i - 1];
        src = &ui_data->chats[i];
        memcpy(dst, src, sizeof(chat_t));
    }

    ui_data->chats = (chat_t *)realloc(ui_data->chats, --ui_data->n_of_chats * sizeof(chat_t));

    if (ui_data->curr_chat)
        ui_data->curr_chat--;
}

void ui_clear_buffer(ui_t *ui_data)
{
    ui_data->buffer[0] = '\0';
    ui_data->text_len = 0;
}

void ui_clear_history(ui_t *ui_data)
{
    if (ui_data->history == NULL)
        return;

    for (int i = 0; i < MAX_HIST; i++)
    {
        if (ui_data->history[i])
            free(ui_data->history[i]);
        ui_data->history[i] = NULL;
    }
    ui_data->hist_head = 0;
}

void ui_append_message(ui_t *ui_data, msg_t *msg)
{
    msg_t *loc_msg = (msg_t *)malloc(msg_size(msg));
    memcpy(loc_msg, msg, msg_size(msg));

    int newhead = (ui_data->hist_head + 1) % ui_data->history_len;
    if (ui_data->history[newhead])
        free(ui_data->history[newhead]);

    ui_data->history[newhead] = loc_msg;
    ui_data->hist_head = newhead;
}

void ui_curr_chat_name(ui_t *ui_data, username_t username)
{
    if (ui_data->n_of_chats < 1)
    {
        username[0] = '\0';
    }
    else
    {
        strncpy(username, ui_data->chats[ui_data->curr_chat].with_user,
                USERNAME_LEN);
    }
}

void ui_close(ui_t *ui_data)
{
    free(ui_data->buffer);
    ui_clear_history(ui_data);
    free(ui_data->history);
    free(ui_data->chats);
    free(ui_data);

    endwin();
}