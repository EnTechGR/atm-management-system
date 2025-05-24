#include <sqlite3.h>
#include "header.h"

// Function to open a connection to the database
sqlite3* open_database(const char* db_path) {
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    
    return db;
}

// Function to close a database connection
void close_database(sqlite3 *db) {
    if (db != NULL) {
        sqlite3_close(db);
    }
}