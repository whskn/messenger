
#ifndef _UI_CONFIG
# define _UI_CONFIG

#define MAX_HIST 50

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
#define HEADER_MAX_LEN 100

#define USERNAME_CHAR       '@'
#define CHATS_BAGE          "YOUR CHATS:"
#define MESSAGE_BAGE        "type your message here"
#define NO_HISTORY          "There is no message history with this user"
#define TOO_SMALL           "window is too small"
#define USERNAME_SHORTAGE   "..."
#define BADNAME_WANRING     "Allowed characters: a-z, A-Z, and 0-9"
#define NO_CHATS            "NO CHATS"
#define CHAT_ADD_HINT       "CTRL + N TO ADD ONE"
#define CHOOSE_CHAT         "Choose a chat to start messaging"

#define ADD_YOURSELF        "You cannot have a chat with yourself"
#define NO_SUCH_USER        "Can't find a user with such name"
#define SERVER_ERROR        "Can't find the user: server error"
#define INVAL_NAME          "Invalid username"
#define USER_EXISTS         "User already exists, choose another name"
#define UNKNOWN_ERR         "Unknown error..."

#define CONNECTING          "Connecting"
#define MAX_DOTS            3


#define BG_HEADER           6
#define FONT_HEADER         7

#define BG_FOOTER           6
#define FONT_FOOTER         7

#define BG_MSG_HEADER       0
#define FONT_MSG_HEADER     5

#define BG_MY_MSG_HEADER    0
#define FONT_MY_MSG_HEAER   1

#define BG_CHAT_SELECTOR    7
#define FONT_CHAT_SELECTOR  0

#define BG_WIN_WARN         1
#define FONT_WIN_WARN       0

#define BG_BADNAME          0
#define FONT_BADNAME        1

#endif