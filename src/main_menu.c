#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void mainMenu(struct User u) {
    char inputBuffer[10];
    int option;
    int validOption = 0;

    while (!validOption) {
        system("clear");
        printf("\n\n\t\t======= ATM =======\n\n");
        printf("\n\t\t-->> Feel free to choose one of the options below <<--\n");
        printf("\n\t\t[1]- Create a new account\n");
        printf("\n\t\t[2]- Update account information\n");
        printf("\n\t\t[3]- Check accounts\n");
        printf("\n\t\t[4]- Check list of owned account\n");
        printf("\n\t\t[5]- Make Transaction\n");
        printf("\n\t\t[6]- Remove existing account\n");
        printf("\n\t\t[7]- Transfer ownership\n");
        printf("\n\t\t[8]- Exit\n");
        printf("Please choose an option (1-8): ");

        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL) {
            printf("\nInput error. Exiting.\n");
            exit(EXIT_FAILURE);
        }

        inputBuffer[strcspn(inputBuffer, "\n")] = 0;

        int i;
        for (i = 0; inputBuffer[i] != '\0'; i++) {
            if (!isdigit(inputBuffer[i])) {
                printf("\nInvalid input! Please enter a number between 1 and 8.\n");
                break;
            }
        }

        if (inputBuffer[i] == '\0') {
            if (sscanf(inputBuffer, "%d", &option) == 1) {
                switch (option) {
                    case 1:
                        createNewAcc(u);
                        validOption = 1; // Set flag to exit the loop after a valid action
                        break;
                    case 2:
                        // student TODO : add your **Update account information** function
                        // here
                        updateAccount(u);
                        validOption = 1;
                        break;
                    case 3:
                        // student TODO : add your **Check the details of existing accounts** function
                        // here
                        checkAccountDetails(u);
                        validOption = 1;
                        break;
                    case 4:
                        checkAllAccounts(u);
                        validOption = 1;
                        break;
                    case 5:
                        // student TODO : add your **Make transaction** function
                        // here
                        makeTransaction(u);
                        validOption = 1;
                        break;
                    case 6:
                        // student TODO : add your **Remove existing account** function
                        // here
                        removeAccount(u);
                        validOption = 1;
                        break;
                    case 7:
                        // student TODO : add your **Transfer owner** function
                        // here
                        transferOwnership(u);
                        validOption = 1;
                        break;
                    case 8:
                        exit(0);
                        break;
                    default:
                        printf("Invalid operation (1-8)!\n");
                }
            } else {
                printf("\nInvalid input! Please enter a number between 1 and 8.\n");
            }
        }

        // If the option was invalid (out of range), we don't set validOption
        // and the loop continues to prompt the user again.
        if (validOption) {
            // Optionally add a pause here before returning to the main menu
            printf("\nPress Enter to return to the main menu...");
            while (getchar() != '\n'); // Consume any remaining input
            getchar(); // Wait for Enter press
        }
    }
}