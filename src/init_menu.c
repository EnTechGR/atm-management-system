#include "header.h"
#include <stdio.h> // Include for standard input/output functions
#include <stdlib.h> // Include for exit()
#include <string.h> // Include for string manipulation (optional, but good practice)
#include <ctype.h>  // Include for character type checking (isdigit)
#include <termios.h>
#include <unistd.h>

static int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);           // Save current terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);         // Disable buffered I/O and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // Apply new settings
    ch = getchar();                           // Read one char (no Enter needed)
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore old settings
    return ch;
}

void getUserById(char username[50], struct User *u) {
    FILE *fp;
    int id;
    char name[50];
    char password[128];
    
    // Initialize with default values
    u->id = -1;
    strcpy(u->name, username);
    
    if ((fp = fopen("./data/users.txt", "r")) == NULL) {
        printf("Error! opening file");
        exit(1);
    }
    
    // Read each line of the users file
    while (fscanf(fp, "%d %s %s", &id, name, password) == 3) {
        if (strcmp(name, username) == 0) {
            // Found the user, populate the struct
            u->id = id;
            strcpy(u->name, name);
            strcpy(u->salt, ""); // We don't need to store the salt/hash here
            strcpy(u->password, ""); // Don't store password in memory
            fclose(fp);
            return;
        }
    }
    
    fclose(fp);
}

void initMenu(struct User *u) {
    char input[10];
    int choice;
    char username[50];
    char password[50];
    
    while (1) {
        system("clear");
        printf("\n\n\n\t\t\t\t  Bank Management System\n\n");
        printf("\n\t\t\t\t1. Login");
        printf("\n\t\t\t\t2. Register");
        printf("\n\t\t\t\t3. Exit");
        printf("\n\n\t\t\t\tEnter your choice: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;  // Remove newline

        int valid = 1;
        for (int i = 0; input[i] != '\0'; i++) {
            if (!isdigit((unsigned char)input[i])) {
                valid = 0;
                break;
            }
        }

        if (!valid || strlen(input) == 0) {
            printf("\n\nInvalid input. Please enter a number.\n");
            printf("\n\nPress any key to continue...");
            getch();
            continue;
        }

        choice = atoi(input); 
        
        switch (choice) {
            case 1:
                if (loginMenu(username, password)) {
                    // If login successful, get the full user details including ID
                    getUserById(username, u);
                    printf("DEBUG - User ID loaded: %d\n", u->id);
                    return;
                }
                break;
            case 2:
                registerMenu(username, password);
                break;
            case 3:
                exit(0);
            default:
                printf("\n\nInvalid choice. Please try again.\n");
                printf("\n\nPress any key to continue...\n");
                getch();
        }
    }
}