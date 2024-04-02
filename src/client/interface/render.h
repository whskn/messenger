#include <time.h>
#include <ncurses.h>

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

    WINDOW* window;
} ui_t;

extern void render_top_bar(ui_t* ui_data);
extern void render_side_bar(ui_t* ui_data);
extern void render_msg_hist(ui_t* ui_data);
extern void clear_footer(ui_t* ui_data);
extern void render_msg_input(ui_t* ui_data);
extern bool check_size(ui_t* ui_data);
extern void render_get_input(ui_t* ui_data, 
                             char* printout, int printout_len,
                             char* input, int input_len, bool badname);