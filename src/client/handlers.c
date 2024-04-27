#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "network.h"
#include "db.h"
#include "config.h"
#include "../message.h"
#include "ui_config.h"
#include "handlers.h"
#include "../misc/validate.h"
#include "ui.h"


void load_history(db_t* db, ui_t* ui_data) {
    int ret;

    ui_clear_history(ui_data);
    if (CURR_CHAT(ui_data) == NULL) return;

    const int with_id = CURR_CHAT(ui_data)->chat_id;
    msg_t* msg;

    int msg_idx = db_count_rows(db, with_id);
    if (MAX_HIST < msg_idx) msg_idx = MAX_HIST;

    while (msg_idx-- > 0) {
        msg = (msg_t*)malloc(sizeof(msg_t));

        ret = db_read_next(db, with_id, msg, msg_idx);
        if (ret == HST_ERROR) {
            free(msg);
            ui_clear_history(ui_data);
            return;
        }

        ui_append_message(ui_data, msg);
    }
}

void chat_up(ui_t* ui_data, db_t* db) {
    ui_switch_chat(ui_data, -1);
    load_history(db, ui_data);
    ui_render_window(ui_data);
}

void chat_down(ui_t* ui_data, db_t* db) {
    ui_switch_chat(ui_data, 1);
    load_history(db, ui_data);
    ui_render_window(ui_data);
}

void send_msg(ui_t* ui_data, connection_t* c, msg_t* msg, db_t* db) {
    // if message field is empty, there is nothing to send
    if (ui_data->buffer[0] == '\0') return;
    // if no chat is selected, user is unknown 
    if (CURR_CHAT(ui_data) == NULL) return;

    int ret = net_send_msg(c, msg, ui_get_buffer(ui_data), 
                           CURR_CHAT(ui_data)->chat_id);
    if (ret <= 0) {
        ui_warning(ui_data, "Failed to send message");
        sleep(WARNING_SLEEP);
        ui_flush_stdin();
        return;
    }

    ui_append_message(ui_data, msg);
    ui_clear_buffer(ui_data);

    ret = db_push(db, msg);
    if (ret != HST_SUCCESS) {
        printf("Failed to save message in database\n");
    }
    
    ui_render_window(ui_data);
}

void backspace(ui_t* ui_data) {
    if (ui_data->text_len > 0) {
        ui_data->buffer[--ui_data->text_len] = '\0';
    }
    
    ui_render_window(ui_data);
}

void chat_request(ui_t* ui_data, connection_t* c) {
    username_t newchat;
    ui_get_input(ui_data, newchat, sizeof(username_t), 
                    "NAME OF THE NEW USER: ", name_filter, false);

    if (!strncmp(newchat, c->my_name, sizeof(username_t))) {
        ui_warning(ui_data, ADD_YOURSELF);
        sleep(WARNING_SLEEP);
        ui_flush_stdin();
        ui_render_window(ui_data);
        return;
    }

    // send user request
    net_user_req(c, newchat);

    ui_render_window(ui_data);
}

void add_char(ui_t* ui_data, char ch) {
    if (CURR_CHAT(ui_data) == NULL) return;
    if (ui_data->text_len < ui_data->buffer_size - 1) { // for new char and \0
        ui_data->buffer[ui_data->text_len++] = ch;
        ui_data->buffer[ui_data->text_len] = '\0';
    }
    
    ui_render_window(ui_data);
}

void incoming_msg(ui_t* ui_data, void* buffer, db_t* db, const int size) {
    int ret;

    if (!msg_is_valid(buffer, size)) {
        printf("Recieved an invalid message...\n");
        ret = -2;
        return;    
    }

    msg_t* msg = (msg_t*)buffer;
    chat_t new_chat;

    if (!db_chat_exists(db, msg->from_id)) {
        // initialize new chat structure
        new_chat.chat_id = msg->from_id;
        memcpy(new_chat.with_user, msg->from_name, sizeof(username_t));

        // add new chat to the db and interface
        ret = db_add_chat(db, &new_chat);
        if (ret != HST_SUCCESS) {
            printf("Failed to insert new chat into db");
            return;
        } 
        ui_add_chat(ui_data, &new_chat);
    }
    chat_t* curr_chat = CURR_CHAT(ui_data);

    ret = db_push(db, msg);
    if (ret != HST_SUCCESS) {
        printf("Failed to save message in database\n");
    }
    if (curr_chat && curr_chat->chat_id == msg->from_id) {
        ui_append_message(ui_data, msg);
    }

    ui_render_window(ui_data);
}

void add_new_chat(ui_t* ui_data, db_t* db, void* buffer) {
    chat_t new_chat;

    user_rsp_t* rsp = (user_rsp_t*)buffer;
    
    if (!rsp->exists) {
        ui_warning(ui_data, NO_SUCH_USER);
        sleep(WARNING_SLEEP);
        ui_flush_stdin();
        ui_render_window(ui_data);
        return;
    }
    if (!name_is_valid(rsp->username)) {
        ui_warning(ui_data, SERVER_ERROR);
        sleep(WARNING_SLEEP);
        ui_flush_stdin();
        ui_render_window(ui_data);
        return;
    }

    new_chat.chat_id = rsp->user_id;
    memcpy(new_chat.with_user, rsp->username, sizeof(username_t));
    ui_add_chat(ui_data, &new_chat);
    load_history(db, ui_data);
    
    ui_render_window(ui_data);
}
