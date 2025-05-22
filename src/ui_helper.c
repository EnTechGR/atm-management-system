#include "header.h"
#include <ctype.h>

#define MAX_INPUT 100

void stayOrReturn(int notGood, void f(struct User u), struct User u) {
    int option;
    if (notGood == 0) {
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
        else {
            printf("Insert a valid operation!\n");
            goto invalid;
        }
    } else {
        printf("\nEnter 1 to go to the main menu and 0 to exit:");
        scanf("%d", &option);
    }
    if (option == 1) {
        system("clear");
        mainMenu(u);
    } else {
        system("clear");
        exit(1);
    }
}

void success(struct User u) {
    char input[MAX_INPUT];
    char *endptr;
    long option;

    printf("\n✔ Success!\n\n");

    while (1) {
        printf("Enter 1 to go to the main menu and 0 to exit:\n");

        // Read full input line
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Error reading input. Try again.\n");
            continue;
        }

        // Remove trailing newline if present
        input[strcspn(input, "\n")] = 0;

        // Use a pointer to traverse input
        char *ptr = input;

        // Skip leading spaces
        while (isspace((unsigned char)*ptr)) ptr++;

        if (*ptr == '\0') {
            printf("Insert a valid operation!\n");
            continue;
        }

        // Convert to long and validate
        option = strtol(ptr, &endptr, 10);

        // If there's leftover non-digit characters, it's invalid
        while (isspace((unsigned char)*endptr)) endptr++; // skip trailing spaces

        if (*endptr != '\0') {
            printf("Insert a valid operation!\n");
            continue;
        }

        system("clear");

        if (option == 1) {
            mainMenu(u);
            break;
        } else if (option == 0) {
            exit(1);
        } else {
            printf("Insert a valid operation!\n");
        }
    }
}