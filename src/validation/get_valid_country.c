#include <ctype.h>
#include "../header.h"

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}



int findCountryByAnyField(const char *input, char *matchedCountry, size_t size) {
    FILE *file = fopen("data/countries.txt", "r");
    if (!file) {
        perror("Error opening countries file");
        return 0;
    }

    char line[256];
    char token[100];
    char lowerInput[100];

    strncpy(lowerInput, input, sizeof(lowerInput) - 1);
    lowerInput[sizeof(lowerInput) - 1] = '\0';
    toLowerCase(lowerInput);

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline

        // Save the original full country name (first field)
        char *firstField = strtok(line, "\t");
        if (!firstField) continue;

        strncpy(token, firstField, sizeof(token) - 1);
        token[sizeof(token) - 1] = '\0';
        char *rest = line + strlen(firstField) + 1;

        // Check first field
        char lowerToken[100];
        strncpy(lowerToken, token, sizeof(lowerToken) - 1);
        lowerToken[sizeof(lowerToken) - 1] = '\0';
        toLowerCase(lowerToken);
        if (strcmp(lowerInput, lowerToken) == 0) {
            strncpy(matchedCountry, token, size - 1);
            matchedCountry[size - 1] = '\0';
            fclose(file);
            return 1;
        }

        // Check the rest of the fields
        char *field = strtok(rest, "\t");
        while (field) {
            strncpy(token, field, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';
            toLowerCase(token);

            if (strcmp(lowerInput, token) == 0) {
                // Match found; return first field (full country name)
                strncpy(matchedCountry, firstField, size - 1);
                matchedCountry[size - 1] = '\0';
                fclose(file);
                return 1;
            }

            field = strtok(NULL, "\t");
        }
    }

    fclose(file);
    return 0; // No match found
}


void getValidCountry(char country[100]) {
    char input[100];
    int valid = 0;

    while (!valid) {
        printf("\nEnter the country:");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            size_t len = strlen(input);
            if (len > 0 && input[len - 1] == '\n') {
                input[len - 1] = '\0';
                len--;
            }

            if (len > 0) {
                int charsOK = 1;
                for (size_t i = 0; i < len; i++) {
                    if (!isalnum(input[i]) && input[i] != ' ' && input[i] != '-') {
                        charsOK = 0;
                        printf("Input should contain only letters, digits, spaces, and hyphens.\n");
                        break;
                    }
                }

                if (charsOK) {
                    char matchedCountry[100];
                    if (findCountryByAnyField(input, matchedCountry, sizeof(matchedCountry))) {
                        valid = 1;
                        strncpy(country, matchedCountry, 99);
                        country[99] = '\0';
                    } else {
                        printf("Invalid country. Not found in the list.\n");
                    }
                }
            } else {
                printf("Country input cannot be empty.\n");
            }
        }
    }
}
