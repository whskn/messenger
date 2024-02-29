#include "interface.h"

void init_ncur() {
    initscr();
    start_color();
    noecho();
    keypad(stdscr, TRUE); 
}

void draw_top_bar() {
    init_pair(1, COLOR_RED, COLOR_BLACK);

    attron(COLOR_PAIR(1));
    mvhline(10, 0, '*', 80);
    attroff(COLOR_PAIR(1));

    refresh();
}

void end_ncur() {
    endwin();
}

void drawChatWin() {

}
