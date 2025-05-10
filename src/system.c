#include "header.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const char *RECORDS = "./data/records.txt";

// Get today's date and validate user input for date
#include "header.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For sleep (if needed for a small delay)

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

// Sanitize and validate country input
void getValidCountry(char country[100]) {
    char input[100];
    int valid = 0;
    // Clear input buffer
    
    while (!valid) {
        fflush(stdin);
        printf("\nEnter the country: ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') {
                input[len-1] = '\0';
                len--;
            }
            
            // Check if input is not empty and contains only letters and spaces
            if (len > 0) {
                valid = 1;
                for (size_t i = 0; i < len; i++) {
                    if (!isalpha(input[i]) && input[i] != ' ' && input[i] != '-') {
                        valid = 0;
                        printf("Country name should contain only letters, spaces, and hyphens.\n");
                        break;
                    }
                }
            } else {
                printf("Country name cannot be empty.\n");
            }
        }
    }
    
    // Copy sanitized input to country
    strncpy(country, input, 99);
    country[99] = '\0'; // Ensure null termination
}

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

// Sanitize and validate account type input
void getValidAccountType(char accountType[10]) {
    char input[20];
    int valid = 0;
    const char *validTypes[] = {"saving", "current", "fixed01", "fixed02", "fixed03"};
    int numTypes = 5;
    
    while (!valid) {
        printf("\nChoose the type of account:\n");
        printf("\t-> saving\n\t-> current\n\t-> fixed01(for 1 year)\n");
        printf("\t-> fixed02(for 2 years)\n\t-> fixed03(for 3 years)\n");
        printf("\n\tEnter your choice: ");
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') {
                input[len-1] = '\0';
                len--;
            }
            
            // Check if input matches any of the valid account types
            for (int i = 0; i < numTypes; i++) {
                if (strcmp(input, validTypes[i]) == 0) {
                    valid = 1;
                    strncpy(accountType, input, 9);
                    accountType[9] = '\0'; // Ensure null termination
                    break;
                }
            }
            
            if (!valid) {
                printf("Invalid account type. Please choose from the options above.\n");
            }
        }
    }
}

// Validate account number
// Validate account number
int getValidAccountNumber() {
    char input[20];
    int accountNbr = 0;
    int valid = 0;

    while (!valid) {
        // Ensure the input buffer is clear before prompting
        fflush(stdin);
        printf("\nEnter the account number: ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') {
                input[len-1] = '\0';
                len--;
            }

            // Check if input contains only digits
            valid = 1;
            for (size_t i = 0; i < len; i++) {
                if (!isdigit(input[i])) {
                    valid = 0;
                    printf("Account number should contain only digits.\n");
                    break;
                }
            }

            // Check if input is not too long and not empty
            if (valid && len > 0 && len <= 10) {
                accountNbr = atoi(input);
                if (accountNbr <= 0) {
                    valid = 0;
                    printf("Account number must be a positive number.\n");
                }
            } else if (len > 10) {
                valid = 0;
                printf("Account number is too long. Maximum 10 digits allowed.\n");
            } else if (len == 0) {
                valid = 0;
                printf("Account number cannot be empty.\n");
            }
        }
    }

    return accountNbr;
}

int getAccountFromFile(FILE *ptr, char name[50], struct Record *r)
{
    return fscanf(ptr, "%d %d %s %d %d/%d/%d %s %s %lf %s",
                &r->id,
		        &r->userId,
		        name,
                &r->accountNbr,
                &r->deposit.month,
                &r->deposit.day,
                &r->deposit.year,
                r->country,
                r->phone,
                &r->amount,
                r->accountType) != EOF;
}

void saveAccountToFile(FILE *ptr, struct User u, struct Record r)
{
    // DEBUGGING: Print values before writing to file
    printf("DEBUG - saveAccountToFile: id=%d, userId=%d, name=%s\n", r.id, r.userId, u.name);
    
    // Ensure we're using the correct userId (from the record, not from somewhere else)
    fprintf(ptr, "%d %d %s %d %d/%d/%d %s %s %.2lf %s\n\n",
            r.id,
            u.id,  // Use u.id directly instead of r.userId which might be corrupted
            u.name,
            r.accountNbr,
            r.deposit.month,
            r.deposit.day,
            r.deposit.year,
            r.country,
            r.phone,
            r.amount,
            r.accountType);
}

void stayOrReturn(int notGood, void f(struct User u), struct User u)
{
    int option;
    if (notGood == 0)
    {
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
        else
        {
            printf("Insert a valid operation!\n");
            goto invalid;
        }
    }
    else
    {
        printf("\nEnter 1 to go to the main menu and 0 to exit:");
        scanf("%d", &option);
    }
    if (option == 1)
    {
        system("clear");
        mainMenu(u);
    }
    else
    {
        system("clear");
        exit(1);
    }
}

void success(struct User u)
{
    int option;
    printf("\n✔ Success!\n\n");
invalid:
    printf("Enter 1 to go to the main menu and 0 to exit!\n");
    scanf("%d", &option);
    system("clear");
    if (option == 1)
    {
        mainMenu(u);
    }
    else if (option == 0)
    {
        exit(1);
    }
    else
    {
        printf("Insert a valid operation!\n");
        goto invalid;
    }
}

void createNewAcc(struct User u) {
    struct Record r;
    struct Record cr;
    char userName[50];
    int lastId = -1;

    // Zero out the entire record structure to clear any garbage values
    memset(&r, 0, sizeof(struct Record));

    // Explicitly set the user ID immediately
    r.userId = u.id;

    // First pass: find the last ID
    FILE *pfRead = fopen(RECORDS, "r");
    if (pfRead != NULL) {
        while (getAccountFromFile(pfRead, userName, &cr)) {
            if (cr.id > lastId) {
                lastId = cr.id;
            }
        }
        fclose(pfRead);
    }

    // Set the new ID (last + 1)
    r.id = lastId + 1;

    // Open file for appending
    FILE *pf = fopen(RECORDS, "a+");
    if (pf == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    noAccount:
    system("clear");
    printf("\t\t\t===== New record =====\n");

    printf("Creating account for user: %s\n", u.name);

    // Show existing account numbers for the user
    printf("Existing account numbers for %s:\n", u.name);
    rewind(pf); // Make sure we're at the start of the file
    int foundAny = 0;
    while (getAccountFromFile(pf, userName, &cr)) {
        if (strcmp(userName, u.name) == 0) {
            printf(" - %d\n", cr.accountNbr);
            foundAny = 1;
        }
    }
    if (!foundAny) {
        printf(" (None)\n");
    }

    // Reset userId just to be absolutely sure
    r.userId = u.id;

    // Use our modified sanitized date input function
    getValidDate(&r.deposit);

    // Get and validate account number
    r.accountNbr = getValidAccountNumber();

    // Reset file position to beginning for checking existing accounts
    rewind(pf);

    // Check for duplicate account numbers for this user
    while (getAccountFromFile(pf, userName, &cr)) {
        if (strcmp(userName, u.name) == 0 && cr.accountNbr == r.accountNbr) {
            printf("✖ This Account already exists for this user\n\n");
            goto noAccount;
        }
    }

    // Get and validate country
    getValidCountry(r.country);

    // Get and validate phone number
    getValidPhone(r.phone);

    // Get and validate deposit amount
    getValidAmount(&r.amount);

    // Get and validate account type
    getValidAccountType(r.accountType);

    // Reset userId one last time
    r.userId = u.id;

    // Move file position to end for appending
    fseek(pf, 0, SEEK_END);

    saveAccountToFile(pf, u, r);
    fclose(pf);
    success(u);
}

void checkAllAccounts(struct User u)
{
    char userName[100];
    struct Record r;

    FILE *pf = fopen(RECORDS, "r");

    system("clear");
    printf("\t\t====== All accounts from user, %s =====\n\n", u.name);
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0)
        {
            printf("_____________________\n");
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:%s \nAmount deposited: $%.2f \nType Of Account:%s\n",
                   r.accountNbr,
                   r.deposit.day,
                   r.deposit.month,
                   r.deposit.year,
                   r.country,
                   r.phone,
                   r.amount,
                   r.accountType);
        }
    }
    fclose(pf);
    success(u);
}
