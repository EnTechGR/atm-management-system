#include <time.h>
#include "../header.h"

// Get today's date and validate user input for date
void getValidDate(struct Date *date) {
    time_t now;
    struct tm *current_time;
    char input[20];
    int day, month, year;
    int valid = 0;

    // Get today's date
    time(&now);
    current_time = localtime(&now);
    int default_day = current_time->tm_mday;
    int default_month = current_time->tm_mon + 1; // tm_mon is 0-11
    int default_year = current_time->tm_year + 1900; // Years since 1900

    // Print the prompt with the current date
    printf("\nEnter today's date(mm/dd/yyyy) or accept default:%02d/%02d/%04d ",
           default_month, default_day, default_year);
    fflush(stdout);

    // Small delay (in seconds) - try if it helps with terminal interaction
    // sleep(0); // Try with 0 first, then maybe a very small value like 0.01 if needed

    // Read the user's input
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove trailing newline if present
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
            len--;
        }

        // If the user just pressed Enter (no modification), the input will be the default
        if (len == 0) {
            date->day = default_day;
            date->month = default_month;
            date->year = default_year;
            valid = 1;
        } else {
            // Try to parse the input
            if (sscanf(input, "%d/%d/%d", &month, &day, &year) == 3) {
                // Validate month (1-12)
                if (month < 1 || month > 12) {
                    printf("Invalid month. Must be between 1 and 12.\n");
                    return getValidDate(date);
                }

                // Validate day based on month (and leap year for February)
                int max_days = 31; // Default for months with 31 days

                if (month == 4 || month == 6 || month == 9 || month == 11) {
                    max_days = 30;
                } else if (month == 2) {
                    // Check for leap year
                    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
                        max_days = 29;
                    } else {
                        max_days = 28;
                    }
                }

                if (day < 1 || day > max_days) {
                    printf("Invalid day for the given month. Must be between 1 and %d.\n", max_days);
                    return getValidDate(date);
                }

                // Validate year (prevent unreasonable dates)
                if (year < 2000 || year > 2100) {
                    printf("Invalid year. Must be between 2000 and 2100.\n");
                    return getValidDate(date);
                }

                // If all validations pass, update the date
                date->day = day;
                date->month = month;
                date->year = year;
                valid = 1;
            } else {
                printf("Invalid date format. Please use mm/dd/yyyy format.\n");
                return getValidDate(date);
            }
        }
    }

    if (!valid) {
        printf("Error reading input. Please try again.\n");
        return getValidDate(date);
    }

    // Confirm the date
    //printf("\nDate set to: %02d/%02d/%04d\n", date->month, date->day, date->year);
}