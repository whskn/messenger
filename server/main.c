#include <stdio.h>
#include <stdbool.h>
#include "network.h"
#include <pthread.h>

//TEHDOLG: with json or other type of cfg file
#define PORT 6969

int main() {
    username_t usernames[MAX_CONNECTIONS];
    conn_t connections[MAX_CONNECTIONS];

    while(true) {
        conn_t connection;
        if (harvestConnection(PORT, &connection, connections, usernames)) {
            printf("Connection harvested\n");
        }

        pthread_t thread;
        MC_arg_t args = {&connection, usernames, connections};
        pthread_create(&thread, 
                       NULL, 
                       &manageConnection, 
                       (void*)&args);
        sleep(1000); // remove
    }

    return 0;
}
