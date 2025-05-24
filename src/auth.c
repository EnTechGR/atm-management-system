#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <unistd.h> 
#include <sqlite3.h>
#include "header.h"
#include "utils/terminal_utils.h"
#include "utils/hashing_utils.h"


int loginMenu(char a[50], char pass[50]) {
    struct termios oflags, nflags;
    struct User user;
    const char *stored_password;

    system("clear");
    printf("\n\n");
    printf("\t+--------------------------------------------------+\n");
    printf("\t|            BANK MANAGEMENT SYSTEM LOGIN          |\n");
    printf("\t+--------------------------------------------------+\n");

    // Username input
    printf("\t|  Username: ");
    fgets(a, 50, stdin);
    a[strcspn(a, "\n")] = 0;
    printf("\t|                                                  |\n");

    // Disable echo for password input
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;
    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    // Read password
     printf("\t|  Password: ");
    fgets(pass, 50, stdin);
    pass[strcspn(pass, "\n")] = 0;
    printf("\t|                                                  |\n");

    // Restore terminal
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
     printf("\t+--------------------------------------------------+\n");
    // Fill user struct
    strncpy(user.name, a, sizeof(user.name) - 1);
    user.name[sizeof(user.name) - 1] = '\0';

    // Fetch stored password hash
    stored_password = getPassword(user);
    if (strcmp(stored_password, "no user found") == 0) {
        printf("\n\t[!] User not found. Please register first.\n");
        printf("\n\tPress any key to continue...");
        getch();
        return 0;
    }

    // Verify the password using the reusable function
    if (verifyPassword(pass, stored_password)) {
        printf("\n\t[✓] Login successful. Welcome, %s!\n", user.name);
        printf("\n\tPress any key to continue...");
        getch();
        return 1;
    } else {
        printf("\n\t[✗] Invalid password. Access denied.\n");
        printf("\n\tPress any key to continue...");
        getch();
        return 0;
    }
}


    const char *getPassword(struct User u) {
    static char stored_password[128];
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return "no user found";
    }

    const char *sql = "SELECT password FROM users WHERE LOWER(name) = LOWER(?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return "no user found";
    }

    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strncpy(stored_password, (const char *)sqlite3_column_text(stmt, 0), sizeof(stored_password) - 1);
        stored_password[sizeof(stored_password) - 1] = '\0';
    } else {
        strcpy(stored_password, "no user found");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return stored_password;
}


int isUsernameTaken(const char *username) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    int result = 0;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    const char *sql = "SELECT 1 FROM users WHERE LOWER(name) = LOWER(?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}


void registerMenu(char a[50], char pass[50]) {
    struct termios oflags, nflags;
    char sanitized_name[50];
    int i;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned char salt[16];
    char hash_hex[65];
    char salt_hex[33];
    char combined_hash[128];
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    system("clear");
    printf("\n\n");
    printf("\t+--------------------------------------------------+\n");
    printf("\t|          BANK MANAGEMENT SYSTEM REGISTRATION     |\n");
    printf("\t+--------------------------------------------------+\n");
    printf("\t|  Enter username: ");
    fgets(a, 50, stdin);
    a[strcspn(a, "\n")] = 0;
     printf("\t|                                                  |\n");

    // Check for invalid characters in username
    for (i = 0; a[i] != '\0'; i++) {
        if (!isalpha(a[i])) {
            printf("\n\t[!] Invalid username. Only a-z, A-Z are allowed.\n");
            printf("\n\tPress any key to continue...");
            getch();
            return;
        }
    }

    // Username is valid, copy it to sanitized_name
    strcpy(sanitized_name, a);

    if (strlen(sanitized_name) == 0) {
        printf("\n\t[!] Username cannot be empty.\n");
        printf("\n\tPress any key to continue...");
        getch();
        return;
    }

    if (isUsernameTaken(sanitized_name)) {
        printf("\n\t[!] Username '%s' is already taken.\n", sanitized_name);
        printf("\n\tPress any key to continue...");
        getch();
        return;
    }

    // Disable echo for password input
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;
    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    char confirm_pass[50];

    printf("\t|  Enter password: ");
    fgets(pass, 50, stdin);
    pass[strcspn(pass, "\n")] = 0;

    printf("\t|  Re-enter password: ");
    fgets(confirm_pass, 50, stdin);
    confirm_pass[strcspn(confirm_pass, "\n")] = 0;

    // Restore terminal settings
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
    printf("\t+--------------------------------------------------+\n");

    if (strcmp(pass, confirm_pass) != 0) {
        printf("\n\t[!] Passwords do not match. Registration aborted.\n");
        printf("\n\tPress any key to continue...");
        getch();
        return;
    }

    if (strchr(pass, ' ') != NULL) {
        
        printf("\n\t[!] Password cannot contain spaces.\n");
        printf("\n\tPress any key to continue...");
        getch();
        return;
    }

    if (strlen(pass) < 8 || strlen(pass) > 12) {
        printf("\n\t[!] Password must be between 8 and 12 characters.\n");
        printf("\n\tPress any key to continue...");
        getch();
        return;
    }

    // Check that password contains only alphanumeric characters
    for (i = 0; pass[i] != '\0'; i++) {
        if (!isalnum(pass[i])) {
            printf("\n\t[!] Password can only contain letters and numbers (no special characters).\n");
            printf("\n\tPress any key to continue...");
            getch();
            return;
        }
    }

    // Generate a random salt
    generateSalt(salt, sizeof(salt));

    // Hash the password with salt
    hashPassword(pass, salt, sizeof(salt), hash);

    // Convert salt and hash to hex strings
    bin2hex(salt, sizeof(salt), salt_hex);
    bin2hex(hash, sizeof(hash), hash_hex);

    // Combine salt and hash for storage (format: salt$hash)
    sprintf(combined_hash, "%s$%s", salt_hex, hash_hex);

    // Database interaction should happen AFTER successful data collection and validation
    rc = sqlite3_open("./data/atm.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "INSERT INTO users (name, password) VALUES (?, ?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, sanitized_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, combined_hash, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute insert statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    printf("\n\t[✓] User '%s' registered successfully!\n", sanitized_name);
    printf("\n\tPress any key to continue...");
    getch();
}