#include "header.h"
#include "database/database.h"
#include "utils/ipc_utils.h"
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

    struct User u;
    memset(&u, 0, sizeof(u));

    // initMenu blocks until a successful login; on choice "Exit" it calls
    // exit(0) directly, so no thread is ever spawned — no leak.
    initMenu(&u);

    // u.name now points into u which lives for the lifetime of main().
    // The thread receives a pointer to that buffer; it is valid until
    // pthread_join() below, which main() always reaches after mainMenu()
    // returns (mainMenu's Exit case calls exit() before we'd get back here,
    // but exit() tears down the process cleanly anyway).
    pthread_t notifThread;
    if (pthread_create(&notifThread, NULL, notificationListener, (void *)u.name) != 0) {
        fprintf(stderr, "Failed to create notification thread\n");
        return -1;
    }

    mainMenu(u);

    // Reached only if mainMenu() returns normally (currently it doesn't —
    // every path either recurses or calls exit()).  Kept here as good
    // practice so that if mainMenu is ever refactored to return, the thread
    // is still joined cleanly.
    pthread_join(notifThread, NULL);

    return 0;
}