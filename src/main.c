#include "header.h"
#include "database/database.h"
#include "utils/ipc_utils.h"
#include "ui/tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <pthread.h>

int main(void)
{
    if (initialize_database("./data/atm.db") != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return -1;
    }

    /* Must be called before any ncurses or TUI function. */
    tui_init();

    struct User u;
    memset(&u, 0, sizeof(u));

    initMenu(&u);

    pthread_t notifThread;
    if (pthread_create(&notifThread, NULL, notificationListener, (void *)u.name) != 0) {
        tui_cleanup();
        fprintf(stderr, "Failed to create notification thread\n");
        return -1;
    }

    mainMenu(u);

    pthread_join(notifThread, NULL);

    tui_cleanup();
    return 0;
}