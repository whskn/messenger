#include <time.h>
#include <ncurses.h>


#define CHAT(ui_data, index) (ui_data->chats_len > 0 && \
                              index < ui_data->chats_len  && \
                              index >= 0\
                              ? ui_data->chats + ui_data->name_size * index \
                              : NULL)
#define CURR_CHAT(ui_data) CHAT(ui_data, ui_data->curr_chat)

typedef struct {
    char* username;
    time_t timestamp;
    int text_len;
    char* buffer;
} ui_msg_t;

typedef struct {
    int* code;
    int bridge_fd;

    char* input_buffer;
    int text_len;
    int buffer_len;

    ui_msg_t* messages;
    int history_len;
    int hist_head;

    char* chats;
    int chats_len;
    int curr_chat;
    int name_size;
    // int chats_array_len;

    WINDOW* window;
} ui_t;

extern void render_top_bar(ui_t* ui_data);
extern void render_side_bar(ui_t* ui_data);
extern void render_msg_hist(ui_t* ui_data);
extern void render_footer(ui_t* ui_data);
extern void render_msg_input(ui_t* ui_data);
extern bool check_size(ui_t* ui_data);
extern void render_get_input(ui_t* ui_data, 
                             char* printout, int printout_len,
                             char* input, int input_len, bool badname);