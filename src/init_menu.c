#include "header.h"
#include <unistd.h>

void initMenu(struct User *u)
{
    int r = 0;
    int option;
    system("clear");
    printf("\n\n\t\t======= ATM =======\n");
    printf("\n\t\t-->> Feel free to login / register :\n");
    printf("\n\t\t[1]- login\n");
    printf("\n\t\t[2]- register\n");
    printf("\n\t\t[3]- exit\n");

    while (!r)
    {
        printf("Please choose an option (1-3): ");
        
        // Check if the input is valid
        if (scanf("%d", &option) != 1)
        {
            // Invalid input handling
            while (getchar() != '\n');  // Clear the input buffer
            printf("\nInvalid input!");
            continue;  // Skip the rest of the loop and prompt the user again
        }
        
        // Clear the input buffer to remove any stray characters
        while (getchar() != '\n' && getchar() != EOF);

        switch (option)
        {
        case 1:
            if (loginMenu(u->name, u->password)) {
                mainMenu(*u); // Go to main menu if login succeeds
            } else {
                // Retry login or go back to main menu
                initMenu(u);
            }
            r = 1;
            break;
        case 2:
            // student TODO : add your **Registration** function here
            registerMenu(u->name, u->password);
            initMenu(u);  // Redirect back to main menu
            r = 1;
            break;
        case 3:
            exit(0); // Exit normally with a code of 0 (indicating successful exit)
            break;
        default:
            printf("Insert a valid operation!\n");
        }
    }
};
