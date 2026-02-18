#include <ctype.h>
#include "../header.h"
#include "validators.h"

int accountNumberInList(int *list, int size, int number) {
    for (int i = 0; i < size; i++) {
        if (list[i] == number) return 1;
    }
    return 0;
}

// Sanitize and validate account type input
void getValidAccountType(char accountType[10]) {
    const char *validTypes[] = {"savings", "current", "fixed01", "fixed02", "fixed03"};
    int numTypes = sizeof(validTypes) / sizeof(validTypes[0]);
    int choice = -1;

    while (choice < 1 || choice > numTypes) {
        printf("\nChoose the type of account:\n");
        printf("  1. savings\n");
        printf("  2. current\n");
        printf("  3. fixed01 (for 1 year)\n");
        printf("  4. fixed02 (for 2 years)\n");
        printf("  5. fixed03 (for 3 years)\n");
        printf("\nEnter your choice [1-%d]: ", numTypes);

        char buffer[10];
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            choice = atoi(buffer);
            if (choice >= 1 && choice <= numTypes) {
                strncpy(accountType, validTypes[choice - 1], 9);
                accountType[9] = '\0'; // Ensure null termination
            } else {
                printf("Invalid selection. Please choose a number between 1 and %d.\n", numTypes);
            }
        }
    }
}

// Validate account number
int getValidAccountNumber(int *existingNumbers, int count) {
    char input[20];
    char err[VALIDATOR_ERR_BUF];
    int value;

    while (1) {
        printf("\nEnter the account number: ");
        if (fgets(input, sizeof(input), stdin) == NULL) continue;
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[--len] = '\0';

        if (val_account_number(input, existingNumbers, count, &value, err))
            return value;
        printf("Error: %s\n", err);
    }
}