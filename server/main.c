#include <stdio.h>
#include <stdbool.h>
#include "server.h"
#include <pthread.h>

//TEHDOLG: with json or other type of cfg file
#define PORT 6969

int main() {
    username_t usernames[MAX_CONNECTIONS];
    conn_t connections[MAX_CONNECTIONS];

    while(true) {
        conn_t connection;
        harvestConnections(PORT, &connection);

        pthread_attr_t 
        pthread_create(manageConnection());
    }

    return 0;
}
