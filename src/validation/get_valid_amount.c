#include "../header.h"
#include "validators.h"

void getValidAmount(double *amount) {
    char input[50];
    char err[VALIDATOR_ERR_BUF];

    while (1) {
        printf("\nEnter amount to deposit: $");
        if (fgets(input, sizeof(input), stdin) == NULL) continue;
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[--len] = '\0';

        if (val_amount(input, amount, err)) return;
        printf("Error: %s\n", err);
    }
}