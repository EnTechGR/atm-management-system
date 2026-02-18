#include <ctype.h>
#include "../header.h"
#include "validators.h"

void getValidPhone(char phone[11]) {
    char input[20];
    char err[VALIDATOR_ERR_BUF];

    while (1) {
        printf("\nEnter the phone number (8-10 digits): ");
        if (fgets(input, sizeof(input), stdin) == NULL) continue;
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[--len] = '\0';

        if (val_phone(input, err)) {
            strncpy(phone, input, 10);
            phone[10] = '\0';
            return;
        }
        printf("Error: %s\n", err);
    }
}