#include "interface.h"

extern void render_size_warning(ui_t* ui_data);
extern void render_top_bar(ui_t* ui_data);
extern void render_empty_side_bar(ui_t* ui_data);
extern void render_side_bar(ui_t* ui_data);
extern void render_msg_hist(ui_t* ui_data);
extern void render_msg_input(ui_t* ui_data);
extern void render_warning(ui_t* ui_data, const char* text);
extern void render_footer(ui_t* ui_data);
extern void render_get_input(ui_t* ui_data, char* printout, int printout_len,
                      char* input, int input_len, bool badname);