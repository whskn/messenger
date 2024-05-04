#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <regex.h>

#include "../misc/validate.h"
#include "config.h"
#include "ui_config.h"
#include "app.h"

bool check_addr(const char *ip, const char *port);

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    username_t my_passwd = {0};
    username_t my_name = {0};
    ui_t *ui_data = NULL;
    chat_t *chats = NULL;
    db_t *db = NULL;
    connection_t c;
    int n_of_chats;
    void *buffer;
    int user_id;
    char *ip;
    int port;
    bool new_acc;

    if (argc < 3)
    {
        printf("Usage: client [server IP] [server PORT]\n");
        exit(1);
    }

    if (!check_addr(argv[1], argv[2]))
    {
        printf("Invalid address\n");
        exit(1);
    }
    ip = argv[1];
    port = atoi(argv[2]);

    if (db_open(NULL, DB_FILENAME, &db) < 0)
    {
        printf("Failed to open database\n");
        exit(1);
    }
    if ((n_of_chats = db_get_chats(db, &chats)) < 0)
    {
        printf("Failed get your contacts\n");
        exit(1);
    }
    if (!(ui_data = ui_init(chats, n_of_chats)))
    {
        printf("Failed to launch interface\n");
        exit(1);
    }
    buffer = (void *)calloc(1, MAX_PACKET_SIZE);
    new_acc = (bool)ui_login(ui_data, &my_name, &my_passwd, name_filter,
                             passwd_filter);

    // loop that re-tries to connect when conn breaks
    ui_warning(ui_data, loading_ani());
    while (true)
    {
        user_id = net_connect(&c, ip, port, my_name, my_passwd, new_acc);
        if (user_id == NET_CHECK_ERRNO ||
            user_id == NET_SERVER_ERROR ||
            user_id == NET_SERVER_OVERLOADED ||
            user_id == NET_CONN_DOWN)
        {
            ui_warning(ui_data, strerror(errno));
            sleep(WARNING_SLEEP);
            continue;
        }
        else if (user_id == NET_TIMEOUT)
        {
            ui_warning(ui_data, loading_ani());
            continue;
        }
        else if (user_id == NET_INVALID_AUTH)
        {
            ui_warning(ui_data, INVAL_NAME);
            sleep(WARNING_SLEEP);
            break;
        }
        else if (user_id == NET_USER_EXISTS)
        {
            ui_warning(ui_data, USER_EXISTS);
            sleep(WARNING_SLEEP);
            break;
        }
        else if (user_id == NET_NO_USER)
        {
            ui_warning(ui_data, NO_SUCH_USER);
            sleep(WARNING_SLEEP);
            break;
        }
        else if (user_id == NET_USER_ONLINE)
        {
            ui_warning(ui_data, USER_ONLINE);
            sleep(WARNING_SLEEP);
            break;
        }
        else if (user_id < 0)
        {
            ui_warning(ui_data, UNKNOWN_ERR);
            sleep(WARNING_SLEEP);
            break;
        }

        new_acc = false;
        ui_flush_stdin();
        ui_set_my_id(ui_data, user_id);
        manage_conn(&c, ui_data, buffer, db);
        net_close_conn(&c);
    }

    free(buffer);
    ui_close(ui_data);
    db_close(db);

    return 0;
}

bool check_addr(const char *ip, const char *port)
{
    int ip_valid, port_valid;
    regex_t regex;

    const char *ip_pattern = "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";
    const char *port_pattern = "^([1-5][0-9]{4}|[6-9][0-9]{3}|[1-9][0-9]{0,3})$";

    if (regcomp(&regex, ip_pattern, REG_EXTENDED))
    {
        printf("Regex error\n");
        exit(1);
    }

    ip_valid = regexec(&regex, ip, 0, NULL, 0);
    regfree(&regex);
    if (ip_valid < 0)
    {
        printf("Regex error\n");
        exit(1);
    }

    if (regcomp(&regex, port_pattern, REG_EXTENDED))
    {
        printf("Regex error\n");
        exit(1);
    }

    port_valid = regexec(&regex, port, 0, NULL, 0);
    regfree(&regex);
    if (port_valid < 0)
    {
        printf("Regex error\n");
        exit(1);
    }

    return !(ip_valid | port_valid);
}