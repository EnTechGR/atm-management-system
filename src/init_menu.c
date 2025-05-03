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
        switch (option)
        {
            case 1:
            loginMenu(u->name, u->password);
            if (strcmp(u->password, getPassword(*u)) == 0)
            {
                printf("\n\nPassword Match!");
                // Retrieve the user's ID
                FILE *fp = fopen("./data/users.txt", "r");
                if (fp) {
                    struct User tempUser;
                    while (fscanf(fp, "%d %s %s", &tempUser.id, tempUser.name, tempUser.password) == 3) {
                        if (strcmp(tempUser.name, u->name) == 0) {
                            u->id = tempUser.id;
                            break;
                        }
                    }
                    fclose(fp);
                    printf(" Logged in with ID: %d\n", u->id); // Confirmation
                    sleep(1);
                    r = 1; // Proceed to the main menu
                } else {
                    perror("Error opening users file");
                    sleep(2);
                    initMenu(u); // Go back to the init menu on error
                    return;
                }
            }
            else
            {
                printf("\nWrong password!! or User Name\n");
                printf("\nPlease try again...\n");
                sleep(2);
                initMenu(u);
                return;
            }
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
