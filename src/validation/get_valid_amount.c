#include "../header.h"

// Sanitize and validate deposit amount input
void getValidAmount(double *amount) {
    char input[50];
    int valid = 0;
    
    while (!valid) {
        printf("\nEnter amount to deposit: $");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') {
                input[len-1] = '\0';
                len--;
            }
            
            // Check if input format is valid for a double
            char *endptr;
            double value = strtod(input, &endptr);
            
            if (endptr != input && *endptr == '\0' && value >= 0.0) {
                // Check for more than two decimal places
                double check;
                if (sscanf(input, "%lf", &check) == 1) {
                    int cents = (int)(check * 100);
                    if ((check * 100) - cents > 0.0001) {
                        printf("Amount should be in correct format (up to two decimal places only).\n");
                        continue;
                    }
                }
                if (value == 0.0) {
                    printf("Amount must be greater than zero.\n");
                    continue;
                }
                *amount = value;
                valid = 1;
            } else {
                printf("Invalid amount. Please enter a positive number.\n");
            }
        }
    }
}