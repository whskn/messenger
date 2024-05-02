#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "../misc/validate.h"
#include "handlers.h"

#define KEY_CTRL(x) (x & 0x1f)
#define KEY_ENTER_REDEF 10
// pulling message history

/**
 * Recieves and sends messages
 *
 * @param c connection with the server
 */
void manage_conn(connection_t *c, ui_t *ui_data, void *buffer, db_t *db)
{
    load_history(db, ui_data);
    ui_render_window(ui_data);
    int ret;

    while (true)
    {
        // polling input fd and socket fd
        struct pollfd fds[2] = {{.fd = c->fd, .events = POLLIN, .revents = 0},
                                {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0}};

        // Blocking until message comes
        ret = poll(fds, (nfds_t)2, -1);
        if (ret == -1 && errno == EINTR)
        {
            ui_render_window(ui_data);
            continue;
        }
        if (ret < 0)
        {
            return;
        }

        if (fds[0].revents)
        {
            if (ret < 0)
                return;

            ret = net_read(c->fd, buffer, MAX_PACKET_SIZE);
            if (ret <= 0)
                return;

            int code = *(int *)buffer;

            switch (code)
            {
            case CC_MSG:
                incoming_msg(ui_data, (msg_t *)buffer, db, ret);
                break;

            case CC_USER_RQS:
                add_new_chat(ui_data, db, (user_rsp_t *)buffer);
                break;

            case NET_CHECK_ERRNO:
                return;

            default:
                net_flush(c);
            }
        }

        if (fds[1].revents)
        {
            if (ret <= 0)
                return;
            int key = getch();

            switch (key)
            {
            case KEY_UP:
                chat_up(ui_data, db);
                break;

            case KEY_DOWN:
                chat_down(ui_data, db);
                break;

            case KEY_ENTER_REDEF:
                send_msg(ui_data, c, (msg_t *)buffer, db);
                break;

            case KEY_BACKSPACE:
                backspace(ui_data);
                break;

            case KEY_CTRL('n'):
                chat_request(ui_data, c);
                break;

            case KEY_RESIZE:
                ui_render_window(ui_data);
                break;

            default:
                add_char(ui_data, (int8_t)key);
            }
        }
    }
}
