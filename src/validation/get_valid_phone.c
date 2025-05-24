#include <ctype.h>
#include "../header.h"

// Sanitize and validate phone number input
void getValidPhone(char phone[11]) {
    char input[20];
    int valid = 0;

    while (!valid) {
        printf("\nEnter the phone number (10 digits): ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline
            size_t len = strlen(input);
            if (len > 0 && input[len - 1] == '\n') {
                input[len - 1] = '\0';
                len--;
            }

            if (len != 10) {
                printf("Phone number must be exactly 10 digits.\n");
                continue;
            }

            valid = 1;
            for (size_t i = 0; i < 10; i++) {
                if (!isdigit(input[i])) {
                    printf("Phone number should contain only digits.\n");
                    valid = 0;
                    break;
                }
            }

            if (valid) {
                strncpy(phone, input, 10);
                phone[10] = '\0'; // Null terminate
            }
        }
    }
}