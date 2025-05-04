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
        scanf("%d", &option);
        // Clear the input buffer to remove the newline character
        while (getchar() != '\n' && getchar() != EOF);

        switch (option)
        {
        case 1:
            if (loginMenu(u->name, u->password)) {
                mainMenu(*u); // Go to main menu if login succeeds
            } else {
                // You can decide to loop back, retry, or just exit
                //printf("\nReturning to main menu...\n");
                //sleep(2);
                initMenu(u);
            }
            r = 1;
            break;
        case 2:
            // student TODO : add your **Registration** function
            // here
            registerMenu(u->name, u->password);
            // printf("\nRegistration Successful!\n");
            // printf("\nPlease login to continue...\n");
            // sleep(2);  // Give user time to read the message
            initMenu(u);  // Redirect back to main menu
            r = 1;
            break;
        case 3:
            exit(1);
            break;
        default:
            printf("Insert a valid operation!\n");
        }
    }
};
