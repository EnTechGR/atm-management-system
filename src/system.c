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

void checkAllAccounts(struct User u)
{
    char userName[100];
    struct Record r;

    FILE *pf = fopen(RECORDS, "r");

    system("clear");
    printf("\t\t====== All accounts from user, %s =====\n\n", u.name);
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0)
        {
            printf("_____________________\n");
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:%s \nAmount deposited: $%.2f \nType Of Account:%s\n",
                   r.accountNbr,
                   r.deposit.day,
                   r.deposit.month,
                   r.deposit.year,
                   r.country,
                   r.phone,
                   r.amount,
                   r.accountType);
        }
    }
    fclose(pf);
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



void makeTransaction(struct User u) {
    struct Record r;
    char userName[50];
    int accountNbr, found = 0, hasAccounts = 0;
    double amount;
    int transactionType; // 1 for deposit, 2 for withdrawal

    FILE *pf = fopen(RECORDS, "r");
    if (pf == NULL) {
        printf("Error opening records file.\n");
        return;
    }

    system("clear");
    printf("======= Available Accounts for %s =======\n", u.name);

    // List user accounts
    while (getAccountFromFile(pf, userName, &r)) {
        if (strcmp(userName, u.name) == 0) {
            printf(" - %d (Type: %s, Balance: $%.2f)\n", r.accountNbr, r.accountType, r.amount);
            hasAccounts = 1;
        }
    }

    if (!hasAccounts) {
        printf("No accounts found.\n");
        fclose(pf);
        success(u);
        return;
    }

    // Get account number for transaction
    printf("\nEnter the account number you want to use for the transaction: ");
    scanf("%d", &accountNbr);
    rewind(pf);

    // Open temp file to rewrite updated records
    FILE *pfTemp = fopen("./data/temp.txt", "w");
    if (pfTemp == NULL) {
        printf("Error creating temporary file.\n");
        fclose(pf);
        return;
    }

    // Process each record
    while (getAccountFromFile(pf, userName, &r)) {
        if (strcmp(userName, u.name) == 0 && r.accountNbr == accountNbr) {
            found = 1;

            // Check account type restrictions
            if (strcmp(r.accountType, "fixed01") == 0 ||
                strcmp(r.accountType, "fixed02") == 0 ||
                strcmp(r.accountType, "fixed03") == 0) {
                printf("✖ Transactions are not allowed on %s accounts.\n", r.accountType);
                fclose(pf);
                fclose(pfTemp);
                remove("./data/temp.txt");
                success(u);
                return;
            }

            // Get transaction type
            printf("\nSelect transaction type:\n1. Deposit\n2. Withdrawal\nEnter choice: ");
            scanf("%d", &transactionType);
            if (transactionType != 1 && transactionType != 2) {
                printf("Invalid choice.\n");
                fclose(pf);
                fclose(pfTemp);
                remove("./data/temp.txt");
                success(u);
                return;
            }

            char amountStr[100];
            int validAmount = 0;

            while (!validAmount) {
                printf("Enter the amount (max 2 decimal places, use '.' not ','): ");
                scanf("%s", amountStr);

                // Check for invalid comma
                if (strchr(amountStr, ',') != NULL) {
                    printf("✖ Use '.' instead of ',' for decimal point.\n");
                    continue;
                }

                // Validate numeric format with at most 2 decimal places
                char *dot = strchr(amountStr, '.');

                if (dot != NULL) {
                    int decimalPlaces = strlen(dot + 1);
                    if (decimalPlaces > 2) {
                        printf("✖ Too many decimal places. Please enter up to 2 decimals only.\n");
                        continue;
                    }
                }

                // Check that the string is a valid number
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

            if (transactionType == 1) {
                r.amount += amount;
                printf("✔ $%.2f deposited successfully.\n", amount);
            } else {
                if (amount > r.amount) {
                    printf("✖ Insufficient funds.\n");
                    fclose(pf);
                    fclose(pfTemp);
                    remove("./data/temp.txt");
                    success(u);
                    return;
                }
                r.amount -= amount;
                printf("✔ $%.2f withdrawn successfully.\n", amount);
            }
        }

        // Write updated or unchanged record
        struct User tempUser = u;
        strncpy(tempUser.name, userName, sizeof(tempUser.name));
        saveAccountToFile(pfTemp, tempUser, r);
    }

    fclose(pf);
    fclose(pfTemp);

    if (!found) {
        printf("✖ Account not found or doesn't belong to you.\n");
        remove("./data/temp.txt");
    } else {
        remove(RECORDS);
        rename("./data/temp.txt", RECORDS);
    }

    success(u);
}

void removeAccount(struct User u) {
    struct Record r;
    char userName[50];
    int accountToRemove;
    int found = 0;

    // Open the file for reading
    FILE *pfRead = fopen(RECORDS, "r");
    if (pfRead == NULL) {
        printf("Error opening file for reading.\n");
        return;
    }

    system("clear");
    printf("======= Remove Account =======\n");

    // List all accounts for the user
    printf("Existing account numbers for %s:\n", u.name);
    int existingAccounts[100];
    int existingCount = 0;
    while (getAccountFromFile(pfRead, userName, &r)) {
        if (strcmp(userName, u.name) == 0) {
            printf(" - %d (Balance: $%.2f, Type: %s)\n", r.accountNbr, r.amount, r.accountType);
            existingAccounts[existingCount++] = r.accountNbr;
            found = 1;
        }
    }

    if (!found) {
        printf("No accounts found for user %s.\n", u.name);
        fclose(pfRead);
        success(u);
        return;
    }

    fclose(pfRead);

    // Ask user to choose account for removal
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
                break;
            }
        }

        if (!validAccount) {
            printf("Account not found. Please try again.\n");
        }
    } while (!validAccount);

    // Open the original file and a temporary file for updating
    FILE *pfReadAgain = fopen(RECORDS, "r");
    FILE *pfTemp = fopen("./data/temp.txt", "w");

    if (pfReadAgain == NULL || pfTemp == NULL) {
        printf("Error opening files.\n");
        if (pfReadAgain) fclose(pfReadAgain);
        if (pfTemp) fclose(pfTemp);
        return;
    }

    // Check if the account has a balance greater than 0
    while (getAccountFromFile(pfReadAgain, userName, &r)) {
        if (strcmp(userName, u.name) == 0 && r.accountNbr == accountToRemove) {
            if (r.amount > 0) {
                printf("This account has a balance of $%.2f. You must withdraw the funds before deletion.\n", r.amount);
                fclose(pfReadAgain);
                fclose(pfTemp);
                success(u);
                return;
            }
        }
    }

    // Rewind file to process and write to temp file
    rewind(pfReadAgain);

    // Process the file and write all records to temp file, excluding the account to remove
    while (getAccountFromFile(pfReadAgain, userName, &r)) {
        if (!(strcmp(userName, u.name) == 0 && r.accountNbr == accountToRemove)) {
            // Use the userName and the user ID from the record itself
            fprintf(pfTemp, "%d %d %s %d %d/%d/%d %s %s %.2lf %s\n\n",
                    r.id,
                    r.userId,
                    userName,
                    r.accountNbr,
                    r.deposit.month,
                    r.deposit.day,
                    r.deposit.year,
                    r.country,
                    r.phone,
                    r.amount,
                    r.accountType);
        }
    }

    fclose(pfReadAgain);
    fclose(pfTemp);

    // Remove the original file and rename the temporary file to replace it
    remove(RECORDS);
    rename("./data/temp.txt", RECORDS);

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

int userExistsInRecords(const char *username) {
    FILE *f = fopen(RECORDS, "r");
    if (!f) return 0;

    struct Record r;
    char recordUsername[50];
    int found = 0;

    while (getAccountFromFile(f, recordUsername, &r)) {
        if (strcmp(recordUsername, username) == 0) {
            found = 1;
            break;
        }
    }

    fclose(f);
    return found;
}

// Helper function to get all the account IDs of a specific user
void getAccountIDsForUser(const char *username, int *accountIDs, int *count) {
    FILE *pfRead = fopen(RECORDS, "r");
    if (!pfRead) {
        printf("Error opening records file.\n");
        return;
    }

    struct Record r;
    char userName[50];
    *count = 0;

    while (getAccountFromFile(pfRead, userName, &r)) {
        if (strcmp(userName, username) == 0) {
            accountIDs[*count] = r.accountNbr;  // Store the account number
            (*count)++;
        }
    }

    fclose(pfRead);
}

void transferOwnership(struct User u) {
    struct Record r;
    char userName[50];
    int accountToTransfer;
    char newOwnerUsername[50];
    int found = 0;

    system("clear");
    printf("======= Transfer Account Ownership =======\n");

    printf("Enter the account number you want to transfer: ");
    if (scanf("%d", &accountToTransfer) != 1) {
        printf("✖ Invalid input.\n");
        while (getchar() != '\n');
        success(u);
        return;
    }

    // Prompt for the new owner's username
    printf("Enter the username of the new owner: ");
    scanf("%s", newOwnerUsername);

    // Check if new owner exists
    if (!userExistsInRecords(newOwnerUsername)) {
        printf("✖ User '%s' does not exist.\n", newOwnerUsername);
        success(u);
        return;
    }

    // Get the list of account IDs for the new owner
    int newOwnerAccountIDs[100];
    int newOwnerAccountCount = 0;
    getAccountIDsForUser(newOwnerUsername, newOwnerAccountIDs, &newOwnerAccountCount);

    // Check if the account number exists in the new owner's account list
    int accountExists = 0;
    for (int i = 0; i < newOwnerAccountCount; i++) {
        if (newOwnerAccountIDs[i] == accountToTransfer) {
            accountExists = 1;
            break;
        }
    }

    if (accountExists) {
        printf("✖ User '%s' already has an account with this ID. Please choose a different account ID.\n", newOwnerUsername);
        success(u);
        return;
    }

    // Read records and write to temp, updating ownership
    FILE *pfRead = fopen(RECORDS, "r");
    FILE *pfTemp = fopen("./data/temp.txt", "w");

    if (pfRead == NULL || pfTemp == NULL) {
        printf("✖ Error accessing account records.\n");
        if (pfRead) fclose(pfRead);
        if (pfTemp) fclose(pfTemp);
        success(u);
        return;
    }

    while (getAccountFromFile(pfRead, userName, &r)) {
        if (strcmp(userName, u.name) == 0 && r.accountNbr == accountToTransfer) {
            found = 1;

            // Update ownership
            strcpy(userName, newOwnerUsername);
            r.userId = getUserIdByName(newOwnerUsername); // Update the user ID for the new owner
        }

        // Save record (updated or not)
        struct User tempUser = u;
        strncpy(tempUser.name, userName, sizeof(tempUser.name));
        saveAccountToFile(pfTemp, tempUser, r);
    }

    fclose(pfRead);
    fclose(pfTemp);

    if (!found) {
        printf("✖ Account not found or doesn't belong to you.\n");
        remove("./data/temp.txt");
    } else {
        remove(RECORDS);
        rename("./data/temp.txt", RECORDS);
        printf("✔ Account %d successfully transferred to %s.\n", accountToTransfer, newOwnerUsername);
    }

    success(u);
}