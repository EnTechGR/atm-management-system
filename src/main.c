#include "header.h"
#include "database.h"
#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <pthread.h>

int main()
{
    if (initialize_database("./data/atm.db") != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return -1;
    }
    struct User u;
    
    initMenu(&u);  // Don't assign return value if void

    pthread_t notifThread;
    if (pthread_create(&notifThread, NULL, notificationListener, (void *)u.name) != 0) {
        fprintf(stderr, "Failed to create notification thread\n");
        return -1;
    }

    mainMenu(u);

    pthread_join(notifThread, NULL);

    return 0;
}
