#include "render.h"
#include <string.h>
#include <stdlib.h>


#define HEADER_HEIGHT 1
#define NAME_LEFT_MARGIN 1
#define FOOTER_LINE_H 1
#define MIN_INPUT_HEIGHT 1
#define MIN_FOOTER_HEIGHT (FOOTER_LINE_H + MIN_INPUT_HEIGHT)
#define LEFT_MSG_MARGIN 0
#define RIGHT_MESSAGE_MARGIN 1
#define RIGHT_CHAT_MARGIN 1
#define USERNAME_CHAR_SIZE 1

#define MIN_WIN_HEIGHT 15
#define MIN_WIN_WIDTH 35
#define MAX_USRENAME_LEN_MSGS 15

#define HIDE_TIME_WIDTH 100
#define SIDE_BAR_DEF_WIDTH 25
#define HIDE_SIDE_BAR_WIDTH 85
#define SIDE_BAR_WIDTH(ui_data) \
    (getmaxx(ui_data->window) > HIDE_SIDE_BAR_WIDTH ? SIDE_BAR_DEF_WIDTH : 0)

#define HEADER_MAX_LEN 100

#define USERNAME_CHAR '@'
#define CHATS_BAGE "YOUR CHATS:"
#define MESSAGE_BAGE "type your message here"
#define NO_HISTORY "no message histroy with this user..."
#define TOO_SMALL "window is too small"
#define USERNAME_SHORTAGE "..."
#define BADNAME_WANRING "Allowed characters: a-z, A-Z, and 0-9"


#define CHAT(ui_data, index) ui_data->chats + ui_data->name_size * index
#define FOOTER_HEIGHT(ui_data, max_x) \
        (MIN_FOOTER_HEIGHT + ui_data->text_len / max_x)


bool check_size(ui_t* ui_data) {
    int width = getmaxx(ui_data->window);
    int height = getmaxy(ui_data->window);

    if (width < MIN_WIN_WIDTH || height < MIN_WIN_HEIGHT) {
        attron(COLOR_PAIR(3));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                mvprintw(y, x, " ");
            }
        }

        mvprintw(height / 2, (width - sizeof(TOO_SMALL)) / 2, TOO_SMALL);
        attroff(COLOR_PAIR(3));
        return false;
    }
    return true;
}

void render_get_input(ui_t* ui_data, char* printout, int printout_len,
                      char* input, int input_len, bool badname) {
    int weight = getmaxx(ui_data->window);
    int height = getmaxy(ui_data->window);

    mvprintw(height / 2 - 2, (weight - printout_len) / 2, printout);
    if (badname) attron(COLOR_PAIR(2));
    mvprintw(height / 2, (weight - input_len) / 2, "%s", input);
    if (badname) {
        attroff(COLOR_PAIR(2));
        mvprintw(height / 2 + 2, (weight - input_len) / 2, "%s", 
                 BADNAME_WANRING);
    }
}

void render_top_bar(ui_t* ui_data) {
    int row = 0;

    attron(COLOR_PAIR(2));
    for (int i = 0; i < getmaxx(ui_data->window); i++) {
        mvprintw(row, i, " ");
    }

    mvprintw(row, NAME_LEFT_MARGIN, "%c%s", USERNAME_CHAR, 
             CHAT(ui_data, ui_data->curr_chat));
    mvprintw(row, getmaxx(ui_data->window) - SIDE_BAR_WIDTH(ui_data), "%s", 
             CHATS_BAGE);
    attroff(COLOR_PAIR(2));
}


void render_side_bar(ui_t* ui_data) {
    if (SIDE_BAR_WIDTH(ui_data) < 1) return;
    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);
    
    int top_margin = HEADER_HEIGHT;
    int col = max_x - SIDE_BAR_WIDTH(ui_data);
    int height = max_y - HEADER_HEIGHT - FOOTER_HEIGHT(ui_data, max_x);

    static int top_idx = 0;

    if (ui_data->curr_chat >= top_idx + height) {
        top_idx = ui_data->curr_chat - height + 1;
    } 
    else if (ui_data->curr_chat < top_idx) {
        top_idx = ui_data->curr_chat;
    }

    for (int i = top_idx; 
          i < ui_data->chats_len && 
          i < height + top_idx && 
          *CHAT(ui_data, i) != '\0'; 
         i++) 
    {
        if (i == ui_data->curr_chat) {
            attron(COLOR_PAIR(1));
        }

        for (int j = 0; j < SIDE_BAR_WIDTH(ui_data); j++) {
            mvprintw(i + top_margin - top_idx, max_x - j, " ");
        }

        if (SIDE_BAR_WIDTH(ui_data) <
            USERNAME_CHAR_SIZE + 
            strlen(CHAT(ui_data, i)) + 
            RIGHT_CHAT_MARGIN) {

            mvprintw(i + top_margin - top_idx, col, "%c%.*s%s", USERNAME_CHAR, 
                     SIDE_BAR_WIDTH(ui_data) - 
                     USERNAME_CHAR_SIZE - 
                     (int)strlen(USERNAME_SHORTAGE) - 
                     RIGHT_MESSAGE_MARGIN, 
                     CHAT(ui_data, i),
                     USERNAME_SHORTAGE);
            }
        else {
            mvprintw(i + top_margin - top_idx, col, "%c%s", USERNAME_CHAR, 
                    CHAT(ui_data, i));
        }

        attroff(COLOR_PAIR(1));
    }
}

int create_header(char* dst, ui_msg_t* m, int width) {
    char* time_tmp = "%02d.%02d.%02d %02d:%02d ";
    struct tm* timeinfo = localtime(&m->timestamp);

    char* name_tmp;
    int username_len = strlen(m->username);

    int cur = 0;

    // printing time
    if (width > HIDE_TIME_WIDTH) {
        cur += sprintf(dst + cur, 
                        time_tmp,
                        timeinfo->tm_mday, 
                        timeinfo->tm_mon + 1,
                        timeinfo->tm_year + 1900, 
                        timeinfo->tm_hour,
                        timeinfo->tm_min);
    }

    // printing username
    if (username_len > MAX_USRENAME_LEN_MSGS) {
        name_tmp = "@%.*s...: ";
        username_len = MAX_USRENAME_LEN_MSGS - 3;
    } 
    else {
        name_tmp = "@%.*s: ";
    }
    cur += sprintf(dst + cur, name_tmp, username_len, m->username);

    return cur;
}

void render_msg_hist(ui_t* ui_data) {
    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);

    const int top_row = HEADER_HEIGHT;
    const int width = max_x - SIDE_BAR_WIDTH(ui_data) - 
                      LEFT_MSG_MARGIN - RIGHT_MESSAGE_MARGIN;
    int row = max_y - FOOTER_HEIGHT(ui_data, max_x) - 1;

    if (ui_data->messages[ui_data->hist_head].buffer == NULL) {
        mvprintw(row, 0, "%s", NO_HISTORY);
        return;
    }

    int i = ui_data->hist_head;
    char* header = (char*)malloc(sizeof(char) * HEADER_MAX_LEN);

    do { // iterate through messages
        ui_msg_t* m = &(ui_data->messages[i]);
        if (m->buffer == NULL) break;

        int header_len = create_header(header, m, getmaxx(ui_data->window));
        
        int line_width = width - header_len;
        int rest_size = m->text_len;

        do {
            int line_len = rest_size % line_width
                            ? rest_size % line_width 
                            : line_width;
            rest_size -= line_len;
            mvprintw(row, LEFT_MSG_MARGIN + header_len, 
                     "%.*s", line_len, m->buffer + rest_size);
        } while (--row >= top_row && rest_size > 0);

        attron(COLOR_PAIR(4));
        mvprintw(row + 1, LEFT_MSG_MARGIN, "%s", header);
        attroff(COLOR_PAIR(4));

        if (--i < 0) i += ui_data->history_len; // same as (i + 1) % history_len

    } while (i != ui_data->hist_head && row >= top_row);

    free(header);
}

void render_msg_input(ui_t* ui_data) {
    int row = getmaxy(ui_data->window) +
              FOOTER_LINE_H - 
              FOOTER_HEIGHT(ui_data, getmaxx(ui_data->window));
    int col = 0;

    if (ui_data->input_buffer[0] == '\0') {
        mvprintw(row, col, MESSAGE_BAGE);
        move(row, col);
    } 
    else {
        mvprintw(row, col, "%s", ui_data->input_buffer);
        move(row, ui_data->text_len);
    }
}

void clear_footer(ui_t* ui_data) {
    const int max_x = getmaxx(ui_data->window);
    const int max_y = getmaxy(ui_data->window);

    int row = max_y - FOOTER_HEIGHT(ui_data, max_x);

    attron(COLOR_PAIR(2));
    for (int l = 0; l < FOOTER_LINE_H; l++) {
        for (int i = 0; i < max_x; i++) {
            mvprintw(row - l, i, " ");
        }
    }
    attroff(COLOR_PAIR(2));
}
