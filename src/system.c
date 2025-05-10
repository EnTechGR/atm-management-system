#include "header.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const char *RECORDS = "./data/records.txt";

// Get today's date and validate user input for date
#include "header.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For sleep (if needed for a small delay)


int getAccountFromFile(FILE *ptr, char name[50], struct Record *r)
{
    return fscanf(ptr, "%d %d %s %d %d/%d/%d %s %s %lf %s",
                &r->id,
		        &r->userId,
		        name,
                &r->accountNbr,
                &r->deposit.month,
                &r->deposit.day,
                &r->deposit.year,
                r->country,
                r->phone,
                &r->amount,
                r->accountType) != EOF;
}

void saveAccountToFile(FILE *ptr, struct User u, struct Record r)
{
    // DEBUGGING: Print values before writing to file
    //printf("DEBUG - saveAccountToFile: id=%d, userId=%d, name=%s\n", r.id, r.userId, u.name);
    
    // Ensure we're using the correct userId (from the record, not from somewhere else)
    fprintf(ptr, "%d %d %s %d %d/%d/%d %s %s %.2lf %s\n\n",
            r.id,
            u.id,  // Use u.id directly instead of r.userId which might be corrupted
            u.name,
            r.accountNbr,
            r.deposit.month,
            r.deposit.day,
            r.deposit.year,
            r.country,
            r.phone,
            r.amount,
            r.accountType);
}

void stayOrReturn(int notGood, void f(struct User u), struct User u)
{
    int option;
    if (notGood == 0)
    {
        system("clear");
        printf("\n✖ Record not found!!\n");
    invalid:
        printf("\nEnter 0 to try again, 1 to return to main menu and 2 to exit:");
        scanf("%d", &option);
        if (option == 0)
            f(u);
        else if (option == 1)
            mainMenu(u);
        else if (option == 2)
            exit(0);
        else
        {
            printf("Insert a valid operation!\n");
            goto invalid;
        }
    }
    else
    {
        printf("\nEnter 1 to go to the main menu and 0 to exit:");
        scanf("%d", &option);
    }
    if (option == 1)
    {
        system("clear");
        mainMenu(u);
    }
    else
    {
        system("clear");
        exit(1);
    }
}

void success(struct User u)
{
    int option;
    printf("\n✔ Success!\n\n");
invalid:
    printf("Enter 1 to go to the main menu and 0 to exit!\n");
    scanf("%d", &option);
    system("clear");
    if (option == 1)
    {
        mainMenu(u);
    }
    else if (option == 0)
    {
        exit(1);
    }
    else
    {
        printf("Insert a valid operation!\n");
        goto invalid;
    }
}

// void createNewAcc(struct User u) {
//     struct Record r;
//     struct Record cr;
//     char userName[50];
//     int lastId = -1;

//     // Zero out the entire record structure to clear any garbage values
//     memset(&r, 0, sizeof(struct Record));

//     // Explicitly set the user ID immediately
//     r.userId = u.id;

//     // First pass: find the last ID
//     FILE *pfRead = fopen(RECORDS, "r");
//     if (pfRead != NULL) {
//         while (getAccountFromFile(pfRead, userName, &cr)) {
//             if (cr.id > lastId) {
//                 lastId = cr.id;
//             }
//         }
//         fclose(pfRead);
//     }

//     // Set the new ID (last + 1)
//     r.id = lastId + 1;

//     // Open file for appending
//     FILE *pf = fopen(RECORDS, "a+");
//     if (pf == NULL) {
//         printf("Error opening file!\n");
//         exit(1);
//     }

//     //noAccount:
//     system("clear");
//     printf("\t\t\t===== New record =====\n");

//     printf("Creating account for user: %s\n", u.name);

//     // Show existing account numbers for the user
//     printf("Existing account numbers for %s:\n", u.name);
//     rewind(pf); // Make sure we're at the start of the file
//     int foundAny = 0;
//     while (getAccountFromFile(pf, userName, &cr)) {
//         if (strcmp(userName, u.name) == 0) {
//             printf(" - %d\n", cr.accountNbr);
//             foundAny = 1;
//         }
//     }
//     if (!foundAny) {
//         printf(" (None)\n");
//     }

//     // Reset userId just to be absolutely sure
//     r.userId = u.id;

//     // Use our modified sanitized date input function
//     getValidDate(&r.deposit);

//     // Get and validate account number
//     r.accountNbr = getValidAccountNumber();

//     // Reset file position to beginning for checking existing accounts
//     rewind(pf);
//     int existingAccounts[100];
//     int existingCount = 0;
//     foundAny = 0;

//     // Check for duplicate account numbers for this user
//     while (getAccountFromFile(pf, userName, &cr)) {
//         if (strcmp(userName, u.name) == 0) {
//             printf(" - %d\n", cr.accountNbr);
//             if (existingCount < 100) {
//                 existingAccounts[existingCount++] = cr.accountNbr;
//             }
//             foundAny = 1;
//         }
//     }

//     if (!foundAny) {
//         printf(" (None)\n");
//     }

//     // Get and validate account number with uniqueness check
//     r.accountNbr = getValidAccountNumber(existingAccounts, existingCount);

//     // Get and validate country
//     getValidCountry(r.country);

//     // Get and validate phone number
//     getValidPhone(r.phone);

//     // Get and validate deposit amount
//     getValidAmount(&r.amount);

//     // Get and validate account type
//     getValidAccountType(r.accountType);

//     // Reset userId one last time
//     r.userId = u.id;

//     // Move file position to end for appending
//     fseek(pf, 0, SEEK_END);

//     saveAccountToFile(pf, u, r);
//     fclose(pf);
//     success(u);
// }

void createNewAcc(struct User u) {
    struct Record r;
    struct Record cr;
    char userName[50];
    int lastId = -1;

    // Clear record memory
    memset(&r, 0, sizeof(struct Record));
    r.userId = u.id;

    // Get the last record ID
    FILE *pfRead = fopen(RECORDS, "r");
    if (pfRead != NULL) {
        while (getAccountFromFile(pfRead, userName, &cr)) {
            if (cr.id > lastId) {
                lastId = cr.id;
            }
        }
        fclose(pfRead);
    }
    r.id = lastId + 1;

    // Open file for reading and appending
    FILE *pf = fopen(RECORDS, "a+");
    if (pf == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    system("clear");
    printf("\t\t\t===== New record =====\n");
    printf("Creating account for user: %s\n", u.name);

    // Show existing account numbers and store them
    rewind(pf);
    int existingAccounts[100];
    int existingCount = 0;
    int foundAny = 0;

    printf("Existing account numbers for %s:\n", u.name);
    while (getAccountFromFile(pf, userName, &cr)) {
        if (strcmp(userName, u.name) == 0) {
            printf(" - %d\n", cr.accountNbr);
            if (existingCount < 100) {
                existingAccounts[existingCount++] = cr.accountNbr;
            }
            foundAny = 1;
        }
    }
    if (!foundAny) {
        printf(" (None)\n");
    }

    // Input steps
    getValidDate(&r.deposit);
    r.accountNbr = getValidAccountNumber(existingAccounts, existingCount);
    getValidCountry(r.country);
    getValidPhone(r.phone);
    getValidAmount(&r.amount);
    getValidAccountType(r.accountType);
    r.userId = u.id;

    // Save and finalize
    fseek(pf, 0, SEEK_END);
    saveAccountToFile(pf, u, r);
    fclose(pf);
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
