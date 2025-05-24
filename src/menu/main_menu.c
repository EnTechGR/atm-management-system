#include "../header.h"
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
        printf("\n");
        printf("\t+------------------------------------------------------+\n");
        printf("\t|                      A T M                           |\n");
        printf("\t+------------------------------------------------------+\n");
        printf("\t|  Feel free to choose one of the options below:       |\n");
        printf("\t+------------------------------------------------------+\n");
        printf("\t| [1] Create a new account                             |\n");
        printf("\t| [2] Update account information                       |\n");
        printf("\t| [3] Check account details                            |\n");
        printf("\t| [4] View list of owned accounts                      |\n");
        printf("\t| [5] Make a transaction                               |\n");
        printf("\t| [6] Remove an existing account                       |\n");
        printf("\t| [7] Transfer account ownership                       |\n");
        printf("\t| [8] Exit                                             |\n");
        printf("\t+------------------------------------------------------+\n");
        printf("\t  Please choose an option (1-8): ");


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