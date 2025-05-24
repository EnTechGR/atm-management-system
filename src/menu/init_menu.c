#include "../header.h"
#include <stdio.h> // Include for standard input/output functions
#include <stdlib.h> // Include for exit()
#include <string.h> // Include for string manipulation (optional, but good practice)
#include <ctype.h>  // Include for character type checking (isdigit)
#include <termios.h>
#include <unistd.h>
#include "../utils/terminal_utils.h"
#include <sqlite3.h>

void getUserById(char username[50], struct User *u) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    u->id = -1; // Default to -1 (not found)
    strncpy(u->name, username, sizeof(u->name));
    u->name[sizeof(u->name) - 1] = '\0';

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "SELECT id FROM users WHERE LOWER(name) = LOWER(?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        u->id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void initMenu(struct User *u) {
    char input[10];
    int choice;
    char username[50];
    char password[50];
    
    while (1) {
        system("clear");
        
        printf("\n\n");
        printf("\t╔════════════════════════════════════════════════════════════╗\n");
        printf("\t║                    WELCOME TO BANK SYSTEM                  ║\n");
        printf("\t╠════════════════════════════════════════════════════════════╣\n");
        printf("\t║                                                            ║\n");
        printf("\t║   [1] ▸ Login                                              ║\n");
        printf("\t║   [2] ▸ Register                                           ║\n");
        printf("\t║   [3] ▸ Exit                                               ║\n");
        printf("\t║                                                            ║\n");
        printf("\t╚════════════════════════════════════════════════════════════╝\n");
        printf("\n\tPlease select an option (1-3): ");

        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;  // Remove newline

        int valid = 1;
        for (int i = 0; input[i] != '\0'; i++) {
            if (!isdigit((unsigned char)input[i])) {
                valid = 0;
                break;
            }
        }

        if (!valid || strlen(input) == 0) {
            printf("\n\t[!] Invalid input. Please enter a number.\n");
            printf("\tPress any key to continue...");
            getch();
            continue;
        }

        choice = atoi(input); 
        
        switch (choice) {
            case 1:
                if (loginMenu(username, password)) {
                    // If login successful, get the full user details including ID
                    getUserById(username, u);
                    return;
                }
                break;
            case 2:
                registerMenu(username, password);
                break;
            case 3:
                exit(0);
            default:
                printf("\n\t[!] Invalid choice. Please try again.\n");
                printf("\tPress any key to continue...");
                getch();
        }
    }
}