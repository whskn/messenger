#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "ui.h"

#define ME_NAMETAG "me"
#define MAX_HIST 50

int handle_input(ui_t* ui_data);
void add_chat(ui_t* ui_data);
void add_chat(ui_t* ui_data);


/**
 * @param chats must be allocated in heap (1d array sizeof(nametype) * len)
 * @param messages must also be fully allocated in heap (2d array)
*/
ui_t* ui_init(int buffer_len, 
              char* chats, 
              int chats_len, 
              const int sizeof_username) {

    // init screen
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // build the struct
    ui_t* ui_data = (ui_t*)malloc(sizeof(ui_t));
    ui_data->input_buffer = (char*)malloc(sizeof(char) * buffer_len);
    ui_data->messages = (ui_msg_t*)calloc(sizeof(ui_msg_t), MAX_HIST);
    ui_data->buffer_len  = buffer_len;
    ui_data->history_len = MAX_HIST;
    ui_data->hist_head   = 0;
    ui_data->chats       = chats;
    ui_data->chats_len   = chats_len;
    ui_data->name_size   = sizeof_username;
    ui_data->curr_chat   = chats_len ? 0 : -1;
    ui_data->text_len    = 0;
    ui_data->window      = stdscr;

    // init colors
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_CYAN);
    init_pair(3, COLOR_BLACK, COLOR_RED);
    init_pair(4, COLOR_RED, COLOR_BLACK);

    // Create an anonymous file descriptor
    int fd = memfd_create("bridge", 0);
    if (fd == -1) {
        perror("memfd_create");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory into the address space
    ftruncate(fd, sizeof(int));
    ui_data->code = \
        mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ui_data->code == MAP_FAILED) {
        // TEHDOLG ERORR HANDLING
        close(fd);
        exit(EXIT_FAILURE);
    }

    *ui_data->code = 0; 
    ui_data->bridge_fd = fd;

    return ui_data;
}

void* ui_handle(void* args) {
    // mapping args
    ui_t* ui_data = args; 

    while (true) {
        while (*ui_data->code);

        if (check_size(ui_data)) {
            clear();
            render_top_bar(ui_data);
            render_side_bar(ui_data);
            render_msg_hist(ui_data);
            render_footer(ui_data);
            render_msg_input(ui_data);
        }
        refresh();

        *ui_data->code |= handle_input(ui_data);
    }
}

bool default_filter(char a __attribute__((unused))) {
    return true;
}

void ui_get_input(ui_t* ui_data, char* data, int size, char* printout, 
                  bool (*_filter)(char)) {
    char* temp_buff = (char*)malloc(size);
    temp_buff[0] = '\0';
    int printout_len = strlen(printout);
    int len = 0;
    bool badname = false;
    bool (*filter)(char) = _filter ? _filter : default_filter;

    while(true) {
        if (!check_size(ui_data)) {
            while (getch() != KEY_RESIZE);
            continue;
        }

        clear();
        render_get_input(ui_data, printout, printout_len, temp_buff, len, badname);

        int ch = getch();
        badname = false;
        switch (ch){
            case '\n':
                for (int i = 0; i < len - 1; i++) {
                    if (!filter(temp_buff[i])) {
                        badname = true;
                        break;
                    }
                }
                if (badname) break;

                memcpy(data, temp_buff, size);
                free(temp_buff);
                return;

            case KEY_RESIZE:
                break;

            case KEY_BACKSPACE:
                if (len > 0) temp_buff[--len] = '\0';
                break;

            default:
                if (len < size - 1) { // for new char and \0
                    temp_buff[len++] = (char)ch;
                    temp_buff[len] = '\0';
                }
                break;
        }
    }
}

void ui_close(ui_t* ui_data) {
    free(ui_data->input_buffer);

    for (int i = 0; i < ui_data->history_len; i++) {
        if (ui_data->messages[i].buffer != NULL) {
            free(ui_data->messages[i].buffer);
        }
        if (ui_data->messages[i].username != NULL) {
            free(ui_data->messages[i].username);
        }
    }
    free(ui_data->messages);
    free(ui_data->chats);
    munmap(ui_data->code, sizeof(int));
    close(ui_data->bridge_fd);

    free(ui_data);

    endwin();
}







void add_chat(ui_t* ui_data) {
    char* username = (char*)malloc(ui_data->name_size);
    username[0] = '\0';

    

    
    free(username);
}

void ui_get_curr_chat(ui_t* ui_data, char* username) {
    if (ui_data->chats_len < 1) {
        username[0] = '\0';
    }
    else {
        memcpy(username, ui_data->chats, ui_data->name_size);
    }
}

void ui_clear_buffer(ui_t* ui_data) {
    ui_data->input_buffer[0] = '\0';
    ui_data->text_len = 0;
}

int handle_input(ui_t* ui_data) {
    int ch = getch();

    switch (ch) {
        case KEY_UP:
            if (ui_data->curr_chat > 0) {
                ui_data->curr_chat = ui_data->curr_chat - 1;
            }
            return CHAT_SWITCH;

        case KEY_DOWN:
            if (ui_data->curr_chat < ui_data->chats_len - 1) {
                ui_data->curr_chat++;
            }
            return CHAT_SWITCH;

        case '\n':
            if (ui_data->input_buffer[0] == '\0') {
                break;
            } 
            if (ui_data->chats_len > 0 && 
                ui_data->curr_chat < ui_data->chats_len && 
                ui_data->curr_chat >= 0) {
                    // TEHDOLG
                    break;
            }
            return MSG_TO_SEND;

        case KEY_BACKSPACE:
            if (ui_data->text_len > 0) {
                ui_data->input_buffer[--ui_data->text_len] = '\0';
            }
            break;

        case KEY_RESIZE:
            break;

        default:
            if (ui_data->text_len < ui_data->buffer_len - 1) { // for new char and \0
                ui_data->input_buffer[ui_data->text_len++] = (char)ch;
                ui_data->input_buffer[ui_data->text_len] = '\0';
            }
            break;
    }

    return NOTHING;
}

void ui_append_message(ui_t* ui_data, time_t timestamp, 
                   char* message, int message_size,
                   char* username) {
    int newhead = (ui_data->hist_head + 1) % ui_data->history_len;
    ui_msg_t* m = &(ui_data->messages[newhead]);
    if (message_size < 1) {
        return;
    }
    
    if (m->buffer != NULL) {
        free(m->buffer);
    }
    if (m->username != NULL) {
        free(m->username);
    }

    m->buffer = (char*)malloc(message_size);
    memcpy(m->buffer, message, message_size);
    m->username = (char*)malloc(ui_data->name_size);
    strncpy(m->username, username, ui_data->name_size);
    m->timestamp = timestamp;
    m->text_len = message_size - 1;

    ui_data->hist_head = newhead;
}

