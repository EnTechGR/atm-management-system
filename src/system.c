#include "header.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For sleep (if needed for a small delay)
#include <termios.h> // For terminal settings
#include <ctype.h>
#include <sqlite3.h>
#include "terminal_utils.h"
#include "file_utils.h"


const char *RECORDS = "./data/records.txt";
sqlite3 *db;

void createNewAcc(struct User u) {
    sqlite3 *db;
    //char *errMsg = 0;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    printf("\t\t\t===== New record =====\n");
    printf("Creating account for user: %s\n", u.name);

    // Show existing account numbers
    printf("Existing account numbers for %s:\n", u.name);
    const char *select_sql = "SELECT account_id FROM accounts WHERE user_id = ?";
    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);
    int existingAccounts[100];
    int existingCount = 0;
    int foundAny = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *acc_id = sqlite3_column_text(stmt, 0);
        printf(" - %s\n", acc_id);
        if (existingCount < 100) {
            existingAccounts[existingCount++] = atoi((const char *)acc_id);
        }
        foundAny = 1;
    }
    if (!foundAny) {
        printf(" (None)\n");
    }

    sqlite3_finalize(stmt);

    // Prepare account record
    struct Record r;
    memset(&r, 0, sizeof(r));
    r.userId = u.id;

    getValidDate(&r.deposit);
    r.accountNbr = getValidAccountNumber(existingAccounts, existingCount);
    getValidCountry(r.country);
    getValidPhone(r.phone);
    getValidAmount(&r.amount);
    getValidAccountType(r.accountType);

    // Insert into DB
    const char *insert_sql =
        "INSERT INTO accounts (user_id, account_id, creation_date, country, phone, balance, account_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    char accountIdStr[20];
    snprintf(accountIdStr, sizeof(accountIdStr), "%d", r.accountNbr);

    char dateStr[11]; // Format date as string
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", r.deposit.year, r.deposit.month, r.deposit.day);

    sqlite3_bind_int(stmt, 1, r.userId);
    sqlite3_bind_text(stmt, 2, accountIdStr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, dateStr, -1, SQLITE_STATIC);  // ← Corrected here
    sqlite3_bind_text(stmt, 4, r.country, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, r.phone, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, r.amount);
    sqlite3_bind_text(stmt, 7, r.accountType, -1, SQLITE_STATIC);


    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
    } else {
        success(u);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void updateAccount(struct User u) {
    sqlite3_stmt *stmt;
    int rc;
    int accountToUpdate;
    int choice;
    int valid = 0;

    // Open database connection
    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    // Step 1: List user's accounts
    printf("======= Update Account =======\n");
    printf("Existing account numbers for %s:\n", u.name);

    const char *queryAccounts = "SELECT account_id FROM accounts WHERE user_id = ?";
    rc = sqlite3_prepare_v2(db, queryAccounts, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);

    int count = 0;
    int existingAccounts[100];

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int accountId = sqlite3_column_int(stmt, 0);
        printf(" - %d\n", accountId);
        if (count < 100) {
            existingAccounts[count++] = accountId;
        }
    }

    sqlite3_finalize(stmt);

    if (count == 0) {
        printf(" (None)\n");
        sqlite3_close(db);  // Close db before returning!
        return;
    }

    // Step 2: Prompt for account number
    do {
        printf("Enter the account number you want to update: ");
        if (scanf("%d", &accountToUpdate) != 1) {
            printf("✖ Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }

        valid = 0;
        for (int i = 0; i < count; i++) {
            if (existingAccounts[i] == accountToUpdate) {
                valid = 1;
                break;
            }
        }

        if (!valid) {
            printf("✖ Invalid account number. Please try again.\n");
        }

    } while (!valid);

    getchar(); // consume newline

    // Step 3: Ask what to update
    printf("What would you like to update?\n");
    printf("1. Country\n");
    printf("2. Phone number\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    getchar(); // consume newline

    char newValue[100];
    const char *updateSQL;

    if (choice == 1) {
        getValidCountry(newValue);
        updateSQL = "UPDATE accounts SET country = ? WHERE user_id = ? AND account_id = ?";
    } else if (choice == 2) {
        getValidPhone(newValue);
        updateSQL = "UPDATE accounts SET phone = ? WHERE user_id = ? AND account_id = ?";
    } else {
        printf("Invalid choice. Returning to main menu...\n");
        sqlite3_close(db);  // Close db before returning!
        return;
    }

    rc = sqlite3_prepare_v2(db, updateSQL, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare update statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, newValue, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, u.id);
    sqlite3_bind_int(stmt, 3, accountToUpdate);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to update account: %s\n", sqlite3_errmsg(db));
    } else {
        printf("✔ Account updated successfully!\n");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);  // Close DB connection here

    success(u);
}

void checkAccountDetails(struct User u) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    int accountNbr;
    int found = 0;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    system("clear");
    printf("======= Your Available Accounts =======\n");

    // List all accounts belonging to the user
    const char *listAccountsSQL = "SELECT account_id FROM accounts WHERE user_id = ?";
    rc = sqlite3_prepare_v2(db, listAccountsSQL, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare list accounts statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);

    int hasAccounts = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int accId = sqlite3_column_int(stmt, 0);
        printf(" - %d\n", accId);
        hasAccounts = 1;
    }
    sqlite3_finalize(stmt);

    if (!hasAccounts) {
        printf(" (None)\n");
        sqlite3_close(db);
        success(u);
        return;
    }

    // Ask user for account number to display details
    printf("\nEnter the account number you want to check in detail: ");
    if (scanf("%d", &accountNbr) != 1) {
        printf("Invalid input!\n");
        while (getchar() != '\n'); // clear input buffer
        sqlite3_close(db);
        return;
    }
    getchar(); // consume newline

    // Query account details for that account number and user
    const char *detailsSQL =
        "SELECT account_id, creation_date, country, phone, balance, account_type "
        "FROM accounts WHERE user_id = ? AND account_id = ?";

    rc = sqlite3_prepare_v2(db, detailsSQL, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare details statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);
    sqlite3_bind_int(stmt, 2, accountNbr);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        found = 1;

        int accId = sqlite3_column_int(stmt, 0);
        const unsigned char *creationDate = sqlite3_column_text(stmt, 1);
        const unsigned char *country = sqlite3_column_text(stmt, 2);
        const unsigned char *phone = sqlite3_column_text(stmt, 3);
        double balance = sqlite3_column_double(stmt, 4);
        const unsigned char *accountType = sqlite3_column_text(stmt, 5);

        // Parse creation_date string into day, month, year
        int year, month, day;
        if (sscanf((const char *)creationDate, "%4d-%2d-%2d", &year, &month, &day) != 3) {
            year = month = day = 0;  // fallback if parsing fails
        }

        printf("\n✔ Account found!\n");
        printf("\nAccount number: %d", accId);
        printf("\nDeposit Date: %d/%d/%d", day, month, year);
        printf("\nCountry: %s", country);
        printf("\nPhone number: %s", phone);
        printf("\nAmount deposited: $%.2f", balance);
        printf("\nType of Account: %s\n", accountType);

        double interestRate = 0.0;
        if (strcmp((const char *)accountType, "savings") == 0) {
            interestRate = 0.07;
        } else if (strcmp((const char *)accountType, "fixed01") == 0) {
            interestRate = 0.04;
        } else if (strcmp((const char *)accountType, "fixed02") == 0) {
            interestRate = 0.05;
        } else if (strcmp((const char *)accountType, "fixed03") == 0) {
            interestRate = 0.08;
        }

        if (interestRate > 0.0) {
            double interest = balance * interestRate / 12; // Monthly interest
            printf("\nYou will get $%.2f as interest on day %d of every month.\n", interest, day);
        } else if (strcmp((const char *)accountType, "current") == 0) {
            printf("\nYou will not get interests because the account is of type current.\n");
        } else {
            printf("\nUnknown account type. Cannot calculate interest.\n");
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (!found) {
        printf("\n✖ Account not found or does not belong to you.\n");
    }

    success(u);
}

void checkAllAccounts(struct User u)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    system("clear");
    printf("\t\t====== All accounts from user, %s =====\n\n", u.name);

    const char *sql = 
        "SELECT account_id, creation_date, country, phone, balance, account_type "
        "FROM accounts WHERE user_id = ?";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);

    int foundAny = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        foundAny = 1;

        int accountNbr = sqlite3_column_int(stmt, 0);
        const unsigned char *creationDate = sqlite3_column_text(stmt, 1);
        const unsigned char *country = sqlite3_column_text(stmt, 2);
        const unsigned char *phone = sqlite3_column_text(stmt, 3);
        double amount = sqlite3_column_double(stmt, 4);
        const unsigned char *accountType = sqlite3_column_text(stmt, 5);

        // Parse creation_date (format YYYY-MM-DD)
        int year = 0, month = 0, day = 0;
        sscanf((const char *)creationDate, "%4d-%2d-%2d", &year, &month, &day);

        printf("_____________________\n");
        printf("\nAccount number: %d\nDeposit Date: %d/%d/%d \nCountry: %s \nPhone number: %s \nAmount deposited: $%.2f \nType Of Account: %s\n",
            accountNbr,
            day,
            month,
            year,
            country,
            phone,
            amount,
            accountType);
    }

    if (!foundAny) {
        printf("No accounts found for user %s.\n", u.name);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    success(u);
}

void makeTransaction(struct User u) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    system("clear");
    printf("======= Available Accounts for %s =======\n", u.name);

    // Query accounts of user
    const char *select_sql = "SELECT account_id, account_type, balance FROM accounts WHERE user_id = ?";
    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);

    int foundAny = 0;
    int accounts[100];
    char accountTypes[100][20];
    double balances[100];
    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        foundAny = 1;
        accounts[count] = sqlite3_column_int(stmt, 0);
        const unsigned char *atype = sqlite3_column_text(stmt, 1);
        strncpy(accountTypes[count], (const char *)atype, sizeof(accountTypes[count]) - 1);
        accountTypes[count][sizeof(accountTypes[count]) - 1] = '\0';
        balances[count] = sqlite3_column_double(stmt, 2);

        printf(" - %d (Type: %s, Balance: $%.2f)\n", accounts[count], accountTypes[count], balances[count]);
        count++;
        if (count >= 100) break;
    }
    sqlite3_finalize(stmt);

    if (!foundAny) {
        printf("No accounts found.\n");
        sqlite3_close(db);
        success(u);
        return;
    }

    int accountNbr;
    printf("\nEnter the account number you want to use for the transaction: ");
    if (scanf("%d", &accountNbr) != 1) {
        printf("Invalid input.\n");
        while (getchar() != '\n'); // clear input buffer
        sqlite3_close(db);
        success(u);
        return;
    }

    // Find selected account index and check restrictions
    int index = -1;
    for (int i = 0; i < count; i++) {
        if (accounts[i] == accountNbr) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("✖ Account not found or does not belong to you.\n");
        sqlite3_close(db);
        success(u);
        return;
    }

    if (strcmp(accountTypes[index], "fixed01") == 0 ||
        strcmp(accountTypes[index], "fixed02") == 0 ||
        strcmp(accountTypes[index], "fixed03") == 0) {
        printf("✖ Transactions are not allowed on %s accounts.\n", accountTypes[index]);
        sqlite3_close(db);
        success(u);
        return;
    }

    int transactionType;
    printf("\nSelect transaction type:\n1. Deposit\n2. Withdrawal\nEnter choice: ");
    if (scanf("%d", &transactionType) != 1 || (transactionType != 1 && transactionType != 2)) {
        printf("Invalid choice.\n");
        while (getchar() != '\n'); // clear input buffer
        sqlite3_close(db);
        success(u);
        return;
    }

    char amountStr[100];
    double amount = 0;
    int validAmount = 0;

    while (!validAmount) {
        printf("Enter the amount (max 2 decimal places, use '.' not ','): ");
        scanf("%s", amountStr);

        if (strchr(amountStr, ',') != NULL) {
            printf("✖ Use '.' instead of ',' for decimal point.\n");
            continue;
        }

        char *dot = strchr(amountStr, '.');
        if (dot != NULL) {
            int decimalPlaces = strlen(dot + 1);
            if (decimalPlaces > 2) {
                printf("✖ Too many decimal places. Please enter up to 2 decimals only.\n");
                continue;
            }
        }

        int isValidFormat = 1;
        for (int i = 0; amountStr[i]; i++) {
            if (!isdigit(amountStr[i]) && amountStr[i] != '.') {
                isValidFormat = 0;
                break;
            }
        }
        if (!isValidFormat) {
            printf("✖ Invalid characters in amount. Use only digits and '.'\n");
            continue;
        }

        amount = atof(amountStr);
        if (amount <= 0) {
            printf("✖ Amount must be a positive number.\n");
            continue;
        }

        validAmount = 1;
    }

    if (transactionType == 2 && amount > balances[index]) {
        printf("✖ Insufficient funds.\n");
        sqlite3_close(db);
        success(u);
        return;
    }

    // Calculate new balance
    double newBalance = balances[index] + (transactionType == 1 ? amount : -amount);

    // Update DB
    const char *update_sql = "UPDATE accounts SET balance = ? WHERE user_id = ? AND account_id = ?";
    rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare update statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_double(stmt, 1, newBalance);
    sqlite3_bind_int(stmt, 2, u.id);
    sqlite3_bind_int(stmt, 3, accountNbr);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to update balance: %s\n", sqlite3_errmsg(db));
    } else {
        if (transactionType == 1) {
            printf("✔ $%.2f deposited successfully.\n", amount);
        } else {
            printf("✔ $%.2f withdrawn successfully.\n", amount);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    success(u);
}

void removeAccount(struct User u) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    system("clear");
    printf("======= Remove Account =======\n");

    // Fetch all user accounts
    const char *select_sql = "SELECT account_id, balance, account_type FROM accounts WHERE user_id = ?";
    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, u.id);

    int existingAccounts[100];
    int existingCount = 0;
    double balances[100];
    char accountTypes[100][20];

    printf("Existing account numbers for %s:\n", u.name);
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int accId = sqlite3_column_int(stmt, 0);
        double bal = sqlite3_column_double(stmt, 1);
        const unsigned char *atype = sqlite3_column_text(stmt, 2);

        printf(" - %d (Balance: $%.2f, Type: %s)\n", accId, bal, atype);

        if (existingCount < 100) {
            existingAccounts[existingCount] = accId;
            balances[existingCount] = bal;
            strncpy(accountTypes[existingCount], (const char *)atype, sizeof(accountTypes[existingCount]) - 1);
            accountTypes[existingCount][sizeof(accountTypes[existingCount]) - 1] = '\0';
            existingCount++;
        }
    }
    sqlite3_finalize(stmt);

    if (existingCount == 0) {
        printf("No accounts found for user %s.\n", u.name);
        sqlite3_close(db);
        success(u);
        return;
    }

    // Ask user to choose account to remove
    int accountToRemove;
    int validAccount = 0;
    do {
        printf("\nEnter the account number you want to remove: ");
        if (scanf("%d", &accountToRemove) != 1) {
            printf("Invalid input. Please enter a valid account number.\n");
            while (getchar() != '\n');  // clear buffer
            continue;
        }

        validAccount = 0;
        for (int i = 0; i < existingCount; i++) {
            if (existingAccounts[i] == accountToRemove) {
                validAccount = 1;
                // Check balance > 0 for this account
                if (balances[i] > 0.0) {
                    printf("This account has a balance of $%.2f. You must withdraw the funds before deletion.\n", balances[i]);
                    validAccount = 0;  // Can't remove this account yet
                }
                break;
            }
        }

        if (!validAccount) {
            printf("Account not found or cannot be removed. Please try again.\n");
        }
    } while (!validAccount);

    // Delete the account from DB
    const char *delete_sql = "DELETE FROM accounts WHERE user_id = ? AND account_id = ?";
    rc = sqlite3_prepare_v2(db, delete_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare delete statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, u.id);
    sqlite3_bind_int(stmt, 2, accountToRemove);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete account: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    printf("✔ Account number %d has been successfully removed.\n", accountToRemove);
    success(u);
}


static int getUserIdByName(const char *username) {
    char filename[100];
    sprintf(filename, "./data/%s.txt", username); // Adjust path if needed
    FILE *f = fopen(filename, "r");
    int id = -1;
    if (f) {
        fscanf(f, "%d", &id); // Assuming user ID is stored as first line in the user file
        fclose(f);
    }
    return id;
}

int userExistsInDB(sqlite3 *db, const char *username) {
    sqlite3_stmt *stmt;
    // Use COLLATE NOCASE for case-insensitive comparison
    const char *sql = "SELECT 1 FROM users WHERE name = ? COLLATE NOCASE LIMIT 1;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int exists = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}


// Helper function to get all the account IDs of a specific user
// Helper: get user ID by username
int getUserIdByNameDB(sqlite3 *db, const char *username) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id FROM users WHERE name = ? LIMIT 1;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int userId = -1;
    if (rc == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return userId;
}

// Helper: get all account IDs for a user by username
void getAccountIDsForUserDB(sqlite3 *db, const char *username, int *accountIDs, int *count) {
    *count = 0;
    int userId = getUserIdByNameDB(db, username);
    if (userId < 0) return;

    sqlite3_stmt *stmt;
    const char *sql = "SELECT account_id FROM accounts WHERE user_id = ?;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int(stmt, 1, userId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*count < 100) {
            accountIDs[*count] = sqlite3_column_int(stmt, 0);
            (*count)++;
        }
    }

    sqlite3_finalize(stmt);
}


void transferOwnership(struct User u) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("./data/atm.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    system("clear");
    printf("======= Transfer Account Ownership =======\n");

    int accountToTransfer;
    printf("Enter the account number you want to transfer: ");
    if (scanf("%d", &accountToTransfer) != 1) {
        printf("✖ Invalid input.\n");
        while (getchar() != '\n');
        success(u);
        sqlite3_close(db);
        return;
    }

    char newOwnerUsername[50];
    printf("Enter the username of the new owner: ");
    scanf("%49s", newOwnerUsername);

    // Check if new owner exists
    if (!userExistsInDB(db, newOwnerUsername)) {
        printf("✖ User '%s' does not exist.\n", newOwnerUsername);
        success(u);
        sqlite3_close(db);
        return;
    }

    // Get new owner's account IDs
    int newOwnerAccountIDs[100];
    int newOwnerAccountCount = 0;
    getAccountIDsForUserDB(db, newOwnerUsername, newOwnerAccountIDs, &newOwnerAccountCount);

    // Check if new owner already has the account ID
    for (int i = 0; i < newOwnerAccountCount; i++) {
        if (newOwnerAccountIDs[i] == accountToTransfer) {
            printf("✖ User '%s' already has an account with this ID. Please choose a different account ID.\n", newOwnerUsername);
            success(u);
            sqlite3_close(db);
            return;
        }
    }

    // Check if the account belongs to the current user
    const char *check_account_sql = "SELECT user_id FROM accounts WHERE account_id = ?;";
    rc = sqlite3_prepare_v2(db, check_account_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        success(u);
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, accountToTransfer);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        printf("✖ Account not found.\n");
        sqlite3_finalize(stmt);
        success(u);
        sqlite3_close(db);
        return;
    }
    int currentOwnerId = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (currentOwnerId != u.id) {
        printf("✖ You do not own this account.\n");
        success(u);
        sqlite3_close(db);
        return;
    }

    // Get new owner ID
    int newOwnerId = getUserIdByNameDB(db, newOwnerUsername);
    if (newOwnerId < 0) {
        printf("✖ Could not find new owner's ID.\n");
        success(u);
        sqlite3_close(db);
        return;
    }

    // Update ownership in DB
    const char *update_sql = "UPDATE accounts SET user_id = ? WHERE account_id = ?;";
    rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare update statement: %s\n", sqlite3_errmsg(db));
        success(u);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, newOwnerId);
    sqlite3_bind_int(stmt, 2, accountToTransfer);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to update account ownership: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        success(u);
        sqlite3_close(db);
        return;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    printf("✔ Account %d successfully transferred to %s.\n", accountToTransfer, newOwnerUsername);
    success(u);
}