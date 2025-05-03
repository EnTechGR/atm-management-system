#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h" // Make sure this header file contains necessary struct definitions

#define ACCOUNTS_FILE "./data/records.txt"

void updateAccountInfo(struct User loggedInUser) {
    int accountIdToUpdate;
    int choice;
    FILE *fp;
    FILE *tempFp;
    char line[256];
    int found = 0;
    int userHasAccounts = 0;

    printf("\n\n\t\t======= Update Account Information =======\n\n");

    // List available accounts for the user
    printf("\n\t\t-->> Available Accounts for User %s <<--\n", loggedInUser.name);
    if ((fp = fopen(ACCOUNTS_FILE, "r")) == NULL) {
        printf("Error opening file.\n");
        printf("\nPress any key to return to the main menu\n");
        getchar();
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        int id, userId, accountId;
        char userName[50], date[20], country[50], phone[20], accountType[20];
        float balance;

        if (sscanf(line, "%d %d %s %d %s %s %s %f %s", &id, &userId, userName, &accountId, date, country, phone, &balance, accountType) == 9) {
            if (userId == loggedInUser.id) {
                printf("\t\tAccount ID: %d\n", accountId);
                userHasAccounts = 1;
            }
        }
    }
    fclose(fp);

    if (!userHasAccounts) {
        printf("\t\tNo accounts found for this user.\n");
        printf("\nPress any key to return to the main menu...\n");
        getchar();
        getchar();
        return;
    }

    printf("\n\t\t-->> Enter the Account ID you want to update <<--\n");
    printf("\t\tAccount ID: ");
    scanf("%d", &accountIdToUpdate);
    getchar();

    printf("\n\t\t-->> Choose the field you want to update <<--\n");
    printf("\t\t[1]- Phone Number\n");
    printf("\t\t[2]- Country\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    getchar();

    if ((fp = fopen(ACCOUNTS_FILE, "r")) == NULL) {
        printf("Error opening file.\n");
        printf("\nPress any key to return to the main menu\n");
        getchar();
        return;
    }

    if ((tempFp = fopen("./data/temp_records.txt", "w")) == NULL) {
        printf("Error opening temporary file.\n");
        fclose(fp);
        printf("\nPress any key to return to the main menu\n");
        getchar();
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        int id, userId, accountId;
        char userName[50], date[20], country[50], phone[20], accountType[20];
        float balance;

        if (sscanf(line, "%d %d %s %d %s %s %s %f %s", &id, &userId, userName, &accountId, date, country, phone, &balance, accountType) == 9) {
            if (accountId == accountIdToUpdate) {
                found = 1;
                char updatedValue[50];
                if (choice == 1) {
                    printf("Enter the new Phone Number: ");
                    scanf("%49s", updatedValue);
                    getchar(); // Consume the newline after reading the phone number.
                    snprintf(line, sizeof(line), "%d %d %s %d %s %s %s %f %s\n", id, userId, userName, accountId, date, country, updatedValue, balance, accountType);
                } else if (choice == 2) {
                    printf("Enter the new Country: ");
                    scanf("%49s", updatedValue);
                    getchar(); // Consume the newline after reading the country.
                    snprintf(line, sizeof(line), "%d %d %s %d %s %s %s %f %s\n", id, userId, userName, accountId, date, updatedValue, phone, balance, accountType);
                } else {
                    printf("Invalid choice.\n");
                }
            }
        }
        fprintf(tempFp, "%s", line);
    }

    fclose(fp);
    fclose(tempFp);

    if (found) {
        remove(ACCOUNTS_FILE);
        rename("./data/temp_records.txt", ACCOUNTS_FILE);
        printf("\nAccount ID %d information updated successfully.\n", accountIdToUpdate);
    } else {
        remove("./data/temp_records.txt");
        printf("\nAccount ID %d not found.\n", accountIdToUpdate);
    }

    printf("\n\nPress any key to return to the main menu...\n");
    getchar();
    getchar();
}