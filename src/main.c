#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Function to initialize database
int initialize_database(const char* db_path) {
    sqlite3 *db;
    int rc;
    char *err_msg = NULL;
    
    // Open database (creates it if it doesn't exist)
    rc = sqlite3_open(db_path, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    
    // SQL statements to create tables
    const char *sql_users = "CREATE TABLE IF NOT EXISTS users ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "name TEXT NOT NULL,"
                          "password TEXT NOT NULL);";
                          
    const char *sql_accounts = "CREATE TABLE IF NOT EXISTS accounts ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "user_id INTEGER NOT NULL,"
                           "account_id TEXT NOT NULL,"
                           "creation_date TEXT NOT NULL,"
                           "country TEXT,"
                           "phone TEXT,"
                           "balance REAL DEFAULT 0.0,"
                           "account_type TEXT,"
                           "FOREIGN KEY (user_id) REFERENCES users(id));";
    
    // Create users table
    rc = sqlite3_exec(db, sql_users, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }
    
    // Create accounts table
    rc = sqlite3_exec(db, sql_accounts, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }
    
    printf("Database initialized successfully.\n");
    sqlite3_close(db);
    return 0;
}

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
