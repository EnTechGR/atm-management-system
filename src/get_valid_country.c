#include <ctype.h>
#include "header.h"

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}



// Check if a given country (lowercased) exists in countries.txt
int isValidCountryInFile(const char *inputCountry) {
    FILE *file = fopen("data/countries.txt", "r");
    if (!file) {
        perror("Error opening countries file");
        return 0;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        // Strip newline and lowercase the line
        line[strcspn(line, "\n")] = '\0';
        toLowerCase(line);

        if (strcmp(inputCountry, line) == 0) {
            fclose(file);
            return 1; // Match found
        }
    }

    fclose(file);
    return 0; // No match
}
// Sanitize and validate country input
void getValidCountry(char country[100]) {
    char input[100];
    int valid = 0;

    while (!valid) {
        printf("\nEnter the country:");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove trailing newline
            size_t len = strlen(input);
            if (len > 0 && input[len - 1] == '\n') {
                input[len - 1] = '\0';
                len--;
            }

            // Check input is non-empty and valid characters only
            if (len > 0) {
                int charsOK = 1;
                for (size_t i = 0; i < len; i++) {
                    if (!isalpha(input[i]) && input[i] != ' ' && input[i] != '-') {
                        charsOK = 0;
                        printf("Country name should contain only letters, spaces, and hyphens.\n");
                        break;
                    }
                }

                if (charsOK) {
                    toLowerCase(input); // Normalize case for checking
                    if (isValidCountryInFile(input)) {
                        valid = 1;
                        strncpy(country, input, 99);
                        country[99] = '\0';
                    } else {
                        printf("Invalid country. Not found in the list.\n");
                    }
                }
            } else {
                printf("Country name cannot be empty.\n");
            }
        }
    }
}