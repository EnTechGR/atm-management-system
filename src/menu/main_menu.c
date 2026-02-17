#include "../header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>

void mainMenu(struct User u) {
    char inputBuffer[10];
    int option;

    // This loop handles the menu display and input validation.
    // Each case calls a function that ends by invoking success(), which
    // already asks the user "Enter 1 to go to the main menu or 0 to exit"
    // and either calls mainMenu() recursively or calls exit().
    // Therefore this loop will only iterate when the user enters an invalid
    // option number — it does NOT need to wait for Enter after a valid action.
    //
    // FIX: removed the "Press Enter to return..." block that appeared after
    //      each valid case.  It was dead code in normal flow (success() calls
    //      mainMenu() before we'd get back here) and caused a double-Enter
    //      requirement in edge cases where the function returned normally.
    while (1) {
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

        inputBuffer[strcspn(inputBuffer, "\n")] = '\0';

        // Validate: must be all digits
        int i;
        int allDigits = 1;
        for (i = 0; inputBuffer[i] != '\0'; i++) {
            if (!isdigit((unsigned char)inputBuffer[i])) {
                allDigits = 0;
                break;
            }
        }

        if (!allDigits || strlen(inputBuffer) == 0) {
            printf("\nInvalid input! Please enter a number between 1 and 8.\n");
            // Small pause so the user can read the error before clear()
            printf("\tPress any key to continue...");
            fflush(stdout);
            // consume one character without requiring Enter
            struct termios oldt, newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            getchar();
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            continue;
        }

        if (sscanf(inputBuffer, "%d", &option) != 1) {
            printf("\nInvalid input! Please enter a number between 1 and 8.\n");
            continue;
        }

        switch (option) {
            case 1: createNewAcc(u);       return;
            case 2: updateAccount(u);      return;
            case 3: checkAccountDetails(u); return;
            case 4: checkAllAccounts(u);   return;
            case 5: makeTransaction(u);    return;
            case 6: removeAccount(u);      return;
            case 7: transferOwnership(u);  return;
            case 8: exit(0);
            default:
                printf("\nInvalid operation! Please choose between 1 and 8.\n");
                break;
        }
    }
}