#include <stdlib.h>
#include <string.h>

#include "render.h"
#include "ui_config.h"

/* Defines the width of side bar based on window width */
#define SIDE_BAR_WIDTH(ui_data) \
    (getmaxx(ui_data->window) > HIDE_SIDE_BAR_WIDTH ? SIDE_BAR_DEF_WIDTH : 0)

/* Defines the height of the footer (text input field) based on text size */
#define FOOTER_HEIGHT(ui_data, max_x) \
    (MIN_FOOTER_HEIGHT + ui_data->text_len / max_x)

void render_size_warning(ui_t *ui_data)
{
    int width = getmaxx(ui_data->window);
    int height = getmaxy(ui_data->window);

    attron(COLOR_PAIR(CLR_WIN_WARN));
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            mvprintw(y, x, " ");
        }
    }

    mvprintw(height / 2, (width - sizeof(TOO_SMALL)) / 2, TOO_SMALL);
    attroff(COLOR_PAIR(CLR_WIN_WARN));
}

void render_login_win(ui_t *ui_data,
                      const char *input_1,
                      const int len_input_1,
                      const char *input_2 __attribute__((unused)),
                      const int len_input_2,
                      const int button,
                      const int field)
{
    const int width = getmaxx(ui_data->window);
    const int height = getmaxy(ui_data->window);

    const int max_line_len = (len_input_1 > len_input_2
                                  ? len_input_1
                                  : len_input_2) +
                             sizeof(USERNAME) - 1;

    const int margin_top = (height - 7) / 2;
    const int margin_left = (width - max_line_len) / 2;

    char *stars = (char *)malloc(len_input_2 + 1);
    memset(stars, '*', len_input_2);
    stars[len_input_2] = '\0';

    mvprintw(margin_top, margin_left, USERNAME);
    printw("%s", input_1);
    mvprintw(margin_top + 2, margin_left, PASSWORD);
    printw("%s", stars);

    if (button == 0)
    {
        attron(COLOR_PAIR(CLR_HEADER));
    }
    mvprintw(margin_top + 4, margin_left, "%s", LOGIN);
    attron(COLOR_PAIR(CLR_NORMAL));

    if (button == 1)
    {
        attron(COLOR_PAIR(CLR_HEADER));
    }
    mvprintw(margin_top + 4, margin_left + sizeof(LOGIN), "%s", REGISTER);
    attron(COLOR_PAIR(CLR_NORMAL));

    attron(COLOR_PAIR(CLR_GRAYED_OUT));
    mvprintw(margin_top + 6, (width - sizeof(INSTRUCTION)) / 2, "%s", INSTRUCTION);
    attroff(COLOR_PAIR(CLR_GRAYED_OUT));

    if (field == 0)
    {
        move(margin_top, margin_left + sizeof(USERNAME) + len_input_1 - 1);
    }
    else if (field == 1)
    {
        move(margin_top + 2, margin_left + sizeof(PASSWORD) + len_input_2 - 1);
    }

    free(stars);
}

void render_get_input(ui_t *ui_data, char *printout, int printout_len,
                      char *input, int input_len, bool badname)
{
    int width = getmaxx(ui_data->window);
    int height = getmaxy(ui_data->window);

    attron(COLOR_PAIR(CLR_GRAYED_OUT));
    mvprintw(height / 2 + 2, (width - sizeof(INSTR_INPUT) - 1) / 2,
             "%s", INSTR_INPUT);
    attron(COLOR_PAIR(CLR_NORMAL));

    mvprintw(height / 2 - 2, (width - printout_len) / 2, printout);
    if (badname)
    {
        attron(COLOR_PAIR(CLR_BADNAME));
        mvprintw(height / 2 + 2, (width - sizeof(BADNAME_WANRING)) / 2, "%s",
                 BADNAME_WANRING);
    }
    mvprintw(height / 2, (width - input_len) / 2, "%s", input);
}

void render_top_bar(ui_t *ui_data)
{
    int row = 0;

    attron(COLOR_PAIR(CLR_HEADER));
    for (int i = 0; i < getmaxx(ui_data->window); i++)
    {
        mvprintw(row, i, " ");
    }

    chat_t *chat = CURR_CHAT(ui_data);
    if (chat != NULL)
    {
        mvprintw(row, NAME_LEFT_MARGIN, "%c%s", USERNAME_CHAR, chat->with_user);
    }
    mvprintw(row, getmaxx(ui_data->window) - SIDE_BAR_WIDTH(ui_data), "%s",
             CHATS_BAGE);
    attroff(COLOR_PAIR(CLR_HEADER));
}

void render_empty_side_bar(ui_t *ui_data)
{
    if (SIDE_BAR_WIDTH(ui_data) < 1)
        return;
    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);

    int top_margin = HEADER_HEIGHT;
    int col = max_x - SIDE_BAR_WIDTH(ui_data);
    int height = max_y - HEADER_HEIGHT - FOOTER_HEIGHT(ui_data, max_x);

    mvprintw(top_margin + height / 2 - 1,
             col + (SIDE_BAR_WIDTH(ui_data) - (sizeof(NO_CHATS) - 1)) / 2,
             NO_CHATS);

    mvprintw(top_margin + height / 2 + 1,
             col + (SIDE_BAR_WIDTH(ui_data) - (sizeof(CHAT_ADD_HINT) - 1)) / 2,
             CHAT_ADD_HINT);
}

void render_side_bar(ui_t *ui_data)
{
    if (SIDE_BAR_WIDTH(ui_data) < 1)
        return;
    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);

    int top_margin = HEADER_HEIGHT;
    int col = max_x - SIDE_BAR_WIDTH(ui_data);
    int height = max_y - HEADER_HEIGHT - FOOTER_HEIGHT(ui_data, max_x);

    if (ui_data->n_of_chats < 1)
    {
        return render_empty_side_bar(ui_data);
    }

    static int top_idx = 0;
    if (ui_data->curr_chat >= top_idx + height)
    {
        top_idx = ui_data->curr_chat - height + 1;
    }
    else if (ui_data->curr_chat < top_idx)
    {
        top_idx = ui_data->curr_chat;
    }

    chat_t *chat;
    for (int i = top_idx;
         i < ui_data->n_of_chats &&
         i < height + top_idx &&
         (chat = CHAT(ui_data, i)) != NULL;
         i++)
    {
        if (i == ui_data->curr_chat)
            attron(COLOR_PAIR(CLR_CHAT_SELECTOR));

        for (int j = 0; j < SIDE_BAR_WIDTH(ui_data); j++)
        {
            mvprintw(i + top_margin - top_idx, max_x - j, " ");
        }

        if (SIDE_BAR_WIDTH(ui_data) <
            USERNAME_CHAR_SIZE +
                strnlen(chat->with_user, USERNAME_LEN) +
                RIGHT_CHAT_MARGIN)
        {

            mvprintw(i + top_margin - top_idx, col, "%c%.*s%s", USERNAME_CHAR,
                     SIDE_BAR_WIDTH(ui_data) -
                         USERNAME_CHAR_SIZE -
                         (int)strlen(USERNAME_SHORTAGE) -
                         RIGHT_MESSAGE_MARGIN,
                     chat->with_user,
                     USERNAME_SHORTAGE);
        }
        else
        {
            mvprintw(i + top_margin - top_idx, col, "%c%s", USERNAME_CHAR,
                     chat->with_user);
        }

        attroff(COLOR_PAIR(CLR_CHAT_SELECTOR));
    }
}

int create_header(char *dst, msg_t *m, int width, char *chatname)
{
    char *time_tmp = "%02d.%02d.%02d %02d:%02d ";
    struct tm *timeinfo = localtime(&m->timestamp);

    char *name_tmp;
    int username_len = strnlen(chatname, USERNAME_LEN);

    int cur = 0;

    // printing time
    if (width > HIDE_TIME_WIDTH)
    {
        cur += sprintf(dst + cur,
                       time_tmp,
                       timeinfo->tm_mday,
                       timeinfo->tm_mon + 1,
                       timeinfo->tm_year + 1900,
                       timeinfo->tm_hour,
                       timeinfo->tm_min);
    }

    // printing username
    if (username_len > MAX_USRENAME_LEN_MSGS)
    {
        name_tmp = "@%.*s...: ";
        username_len = MAX_USRENAME_LEN_MSGS - 3;
    }
    else
    {
        name_tmp = "@%.*s: ";
    }
    cur += sprintf(dst + cur, name_tmp, username_len, chatname);

    return cur;
}

void render_msg_hist(ui_t *ui_data)
{
    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);

    const int top_row = HEADER_HEIGHT;
    const int width = max_x - SIDE_BAR_WIDTH(ui_data) -
                      LEFT_MSG_MARGIN - RIGHT_MESSAGE_MARGIN;
    int row = max_y - FOOTER_HEIGHT(ui_data, max_x) - 1;
    int head;
    char *header;

    if (CURR_CHAT(ui_data) == NULL)
    {
        mvprintw(max_y / 2 + HEADER_HEIGHT,
                 (width - (sizeof(CHOOSE_CHAT) - 1)) / 2,
                 "%s", CHOOSE_CHAT);
        return;
    }
    if (ui_data->history[ui_data->hist_head] == NULL)
    {
        mvprintw(row / 2 + HEADER_HEIGHT,
                 (width - (sizeof(NO_HISTORY) - 1)) / 2,
                 "%s", NO_HISTORY);
        return;
    }

    head = ui_data->hist_head;
    header = (char *)malloc(sizeof(char) * HEADER_MAX_LEN);

    /* iterate through message history */
    do
    {
        msg_t *m = ui_data->history[head];
        if (m == NULL)
        {
            break;
        }
        bool my_message = (m->from_id == ui_data->my_id);

        int header_len = create_header(header,
                                       m,
                                       getmaxx(ui_data->window),
                                       my_message
                                           ? ME_NAMETAG
                                           : CURR_CHAT(ui_data)->with_user);

        int line_width = width - header_len;
        int rest_size = m->text_size - 1; // _len = _size - 1, cause of \0

        /* iterate through lines of message */
        do
        {
            int line_len = rest_size % line_width
                               ? rest_size % line_width
                               : line_width;
            rest_size -= line_len;
            mvprintw(row, LEFT_MSG_MARGIN + header_len,
                     "%.*s", line_len, m->buffer + rest_size);
        } while (--row >= top_row && rest_size > 0);

        if (my_message)
        {
            attron(COLOR_PAIR(CLR_MY_MSG_HEADER));
        }
        else
        {
            attron(COLOR_PAIR(CLR_MSG_HEADER));
        }
        mvprintw(row + 1, LEFT_MSG_MARGIN, "%s", header);
        attroff(COLOR_PAIR(CLR_BADNAME));

        /* same as (i + 1) % history_len */
        if (--head < 0)
        {
            head += ui_data->history_len;
        }

    } while (head != ui_data->hist_head && row >= top_row);

    free(header);
}

void render_msg_input(ui_t *ui_data)
{
    if (CURR_CHAT(ui_data) == NULL)
        return;

    int row = getmaxy(ui_data->window) +
              FOOTER_LINE_H -
              FOOTER_HEIGHT(ui_data, getmaxx(ui_data->window));
    int col = 0;

    if (ui_data->buffer[0] == '\0')
    {
        mvprintw(row, col, MESSAGE_BAGE);
        move(row, col);
    }
    else
    {
        mvprintw(row, col, "%s", ui_data->buffer);
        move(row, ui_data->text_len);
    }
}

void render_footer(ui_t *ui_data)
{
    if (CURR_CHAT(ui_data) == NULL)
        return;

    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);

    int row = max_y - FOOTER_HEIGHT(ui_data, max_x);

    attron(COLOR_PAIR(CLR_FOOTER));
    for (int l = 0; l < FOOTER_LINE_H; l++)
    {
        for (int i = 0; i < max_x; i++)
        {
            mvprintw(row - l, i, " ");
        }
    }
    if (max_x >= (int)sizeof(FOOTER_STR))
    {
        mvprintw(row, 0, FOOTER_STR);
    }
    attroff(COLOR_PAIR(CLR_FOOTER));
}

void render_warning(ui_t *ui_data, const char *text)
{
    const int width = getmaxx(ui_data->window);
    const int height = getmaxy(ui_data->window);
    const int printout_len = strlen(text);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            mvprintw(y, x, " ");
        }
    }

    mvprintw(height / 2, (width - printout_len) / 2, text);
}