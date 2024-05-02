/**
 * This header file provides functions to render particular elements
 * of the user interface with ncurses. All of the functions DO NOT
 * accept user input or change window contents dynamically. All they do is
 * just render UI parts.
 *
 * Only for use in ui.c
 */

#include "interface.h"

extern void render_size_warning(ui_t *ui_data);
extern void render_top_bar(ui_t *ui_data);
extern void render_empty_side_bar(ui_t *ui_data);
extern void render_side_bar(ui_t *ui_data);
extern void render_msg_hist(ui_t *ui_data);
extern void render_msg_input(ui_t *ui_data);
extern void render_warning(ui_t *ui_data, const char *text);
extern void render_footer(ui_t *ui_data);
extern void render_get_input(ui_t *ui_data, char *printout, int printout_len,
                             char *input, int input_len, bool badname);
extern void render_login_win(ui_t *ui_data,
                             const char *input_1,
                             const int len_input_1,
                             const char *input_2,
                             const int len_input_2,
                             const int button,
                             const int field);