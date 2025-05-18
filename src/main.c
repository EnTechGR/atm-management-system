#include "header.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>



int main()
{
    if (initialize_database("./data/atm.db") != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return -1;
    }

    struct User u;
    
    initMenu(&u);
    mainMenu(u);
    return 0;
}
