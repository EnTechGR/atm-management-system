#include "header.h"
#include <stdio.h> // Include for standard input/output functions
#include <stdlib.h> // Include for exit()
#include <string.h> // Include for string manipulation (optional, but good practice)
#include <ctype.h>  // Include for character type checking (isdigit)

void initMenu(struct User *u) {
    int r = 0;
    char inputBuffer[10]; // Increased buffer size for safety
    int option;

    system("clear");
    printf("\n\n\t\t======= ATM =======\n");
    printf("\n\t\t-->> Feel free to login / register :\n");
    printf("\n\t\t[1]- login\n");
    printf("\n\t\t[2]- register\n");
    printf("\n\t\t[3]- exit\n");

    while (!r) {
        printf("Please choose an option (1-3): ");

        // Read the entire line of input as a string
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL) {
            // Handle potential input error (e.g., EOF)
            printf("\nInput error. Exiting.\n");
            exit(EXIT_FAILURE);
        }

        // Remove trailing newline character if present
        inputBuffer[strcspn(inputBuffer, "\n")] = 0;

        // Check if the input consists only of digits
        int i;
        for (i = 0; inputBuffer[i] != '\0'; i++) {
            if (!isdigit(inputBuffer[i])) {
                printf("\nInvalid input! Please enter a number between 1 and 3.\n");
                break;
            }
        }

        // If the input is all digits, try to convert it to an integer
        if (inputBuffer[i] == '\0') {
            if (sscanf(inputBuffer, "%d", &option) == 1) {
                switch (option) {
                    case 1:
                        if (loginMenu(u->name, u->password)) {
                            mainMenu(*u); // Go to main menu if login succeeds
                        } else {
                            initMenu(u); // Retry login
                        }
                        r = 1;
                        break;
                    case 2:
                        registerMenu(u->name, u->password);
                        initMenu(u); // Redirect back to main menu
                        r = 1;
                        break;
                    case 3:
                        exit(0); // Exit normally
                        break;
                    default:
                        printf("Insert a valid operation (1-3)!\n");
                }
            } else {
                printf("\nInvalid input! Please enter a number between 1 and 3.\n");
            }
        }
    }
}