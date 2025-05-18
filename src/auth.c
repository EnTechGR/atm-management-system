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
#include "header.h" // Assuming header.h contains the definition for struct User


int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);           // Save current terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);         // Disable buffered I/O and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // Apply new settings
    ch = getchar();                           // Read one char (no Enter needed)
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore old settings
    return ch;
}


// Helper function to convert a string to lowercase
void toLower(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Function to generate a random salt
void generateSalt(unsigned char *salt, size_t length) {
    if (RAND_bytes(salt, length) != 1) {
        // If RAND_bytes fails, use a fallback method
        srand(time(NULL));
        for (size_t i = 0; i < length; i++) {
            salt[i] = rand() & 0xff;
        }
    }
}

// Function to convert binary data to hexadecimal string
void bin2hex(unsigned char *bin, size_t bin_len, char *hex) {
    static const char hexchars[] = "0123456789abcdef";
    for (size_t i = 0; i < bin_len; i++) {
        hex[i*2] = hexchars[(bin[i] >> 4) & 0xF];
        hex[i*2+1] = hexchars[bin[i] & 0xF];
    }
    hex[bin_len*2] = '\0';
}

// Function to hash a password with a salt using SHA-256
void hashPassword(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *hash) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        perror("EVP_MD_CTX_new");
        exit(1);
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) ||
        1 != EVP_DigestUpdate(mdctx, password, strlen(password)) ||
        1 != EVP_DigestUpdate(mdctx, salt, salt_len) ||
        1 != EVP_DigestFinal_ex(mdctx, hash, NULL)) {
        perror("EVP_Digest functions");
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    EVP_MD_CTX_free(mdctx);
}

// Function to verify a password against a stored hash
int verifyPassword(const char *password, const char *stored_password) {
    char salt_hex[33];
    unsigned char salt[16];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char computed_hash_hex[65];
    char *hash_part;
    
    // Extract the salt from the stored password
    // Format of stored_password is: salt_hex$hash_hex
    strncpy(salt_hex, stored_password, 32);
    salt_hex[32] = '\0';
    
    // Find the hash part (after the $)
    hash_part = strchr(stored_password, '$');
    if (hash_part == NULL) {
        // Invalid format, possibly still using old plaintext format
        return strcmp(password, stored_password) == 0;
    }
    hash_part++; // Skip the $ character
    
    // Convert salt from hex to binary
    for (int i = 0; i < 16; i++) {
        sscanf(&salt_hex[i*2], "%2hhx", &salt[i]);
    }
    
    // Hash the provided password with the extracted salt
    hashPassword(password, salt, sizeof(salt), hash);
    
    // Convert the computed hash to hex
    bin2hex(hash, sizeof(hash), computed_hash_hex);
    
    // Compare the computed hash with the stored hash
    return strcmp(computed_hash_hex, hash_part) == 0;
}

int loginMenu(char a[50], char pass[50]) {
    struct termios oflags, nflags;
    struct User user;
    const char *stored_password;
    system("clear");
    printf("\n\n\n\t\t\t\t  Bank Management System\n\t\t\t\t\t User Login:");
    // Read username
    printf("\n\nEnter username: ");
    fgets(a, 50, stdin);
    a[strcspn(a, "\n")] = 0;
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
    printf("\nEnter password: ");
    fgets(pass, 50, stdin);
    pass[strcspn(pass, "\n")] = 0;
    // Restore terminal
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
    // Fill user struct and check password
    strncpy(user.name, a, sizeof(user.name) - 1);
    user.name[sizeof(user.name) - 1] = '\0';
    stored_password = getPassword(user);
    if (strcmp(stored_password, "no user found") == 0) {
        printf("\n\nUser not found. Please register first.\n");
        printf("\n\nPress any key to continue...");
        getch();
        return 0; // login failed
    } else if (verifyPassword(pass, stored_password)) {
        printf("\n\nLogin successful. Welcome, %s!\n", user.name);
        printf("\n\nPress any key to continue...");
        getch();  // flush leftover input
        //getchar();
        return 1; // login successful
    } else {
        printf("\n\nInvalid password. Access denied.\n");
        printf("\n\nPress any key to continue...");
        getch();
        return 0; // login failed
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
    printf("\n\n\n\t\t\t\t\t Bank Management System\n\t\t\t\t\t User Registration:");
    printf("\n\nEnter the user name:");
    fgets(a, 50, stdin);
    a[strcspn(a, "\n")] = 0;

    // Check for invalid characters in username
    for (i = 0; a[i] != '\0'; i++) {
        if (!isalnum(a[i])) {
            printf("\n\nInvalid username. Only alphanumeric characters (a-z, A-Z, 0-9) are allowed.\n");
            printf("\n\nPress any key to continue...");
            getch();
            return;
        }
    }

    // Username is valid, copy it to sanitized_name
    strcpy(sanitized_name, a);

    if (strlen(sanitized_name) == 0) {
        printf("\n\nUsername cannot be empty.\n");
        printf("\n\nPress any key to continue...");
        getch();
        return;
    }

    if (isUsernameTaken(sanitized_name)) {
        printf("\n\nUsername '%s' is already taken (case-insensitive). Please choose a different username.\n", sanitized_name);
        printf("\n\nPress any key to continue...");
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

    printf("\n\nEnter the password:");
    fgets(pass, 50, stdin);
    pass[strcspn(pass, "\n")] = 0;

    printf("\nRe-enter the password:");
    fgets(confirm_pass, 50, stdin);
    confirm_pass[strcspn(confirm_pass, "\n")] = 0;

    // Restore terminal settings
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    if (strcmp(pass, confirm_pass) != 0) {
        printf("\n\nPasswords do not match. Registration aborted.\n");
        printf("\n\nPress any key to continue...");
        getch();
        return;
    }

    if (strchr(pass, ' ') != NULL) {
        printf("\n\nPassword cannot contain spaces. Registration aborted.\n");
        printf("\n\nPress any key to continue...");
        getch();
        return;
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

    printf("\n\nUser %s registered successfully!\n", sanitized_name);
    printf("\n\nPress any key to continue...");
    getch();
}