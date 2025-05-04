#include "header.h"

const char *RECORDS = "./data/records.txt";

int getAccountFromFile(FILE *ptr, char name[50], struct Record *r)
{
    return fscanf(ptr, "%d %d %s %d %d/%d/%d %s %d %lf %s",
                  &r->id,
		  &r->userId,
		  name,
                  &r->accountNbr,
                  &r->deposit.month,
                  &r->deposit.day,
                  &r->deposit.year,
                  r->country,
                  &r->phone,
                  &r->amount,
                  r->accountType) != EOF;
}

void saveAccountToFile(FILE *ptr, struct User u, struct Record r)
{
    // DEBUGGING: Print values before writing to file
    printf("DEBUG - saveAccountToFile: id=%d, userId=%d, name=%s\n", r.id, r.userId, u.name);
    
    // Ensure we're using the correct userId (from the record, not from somewhere else)
    fprintf(ptr, "%d %d %s %d %d/%d/%d %s %d %.2lf %s\n\n",
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

void createNewAcc(struct User u)
{
    struct Record r;
    struct Record cr;
    char userName[50];
    int lastId = -1;
    
    // DEBUGGING: Print the user ID at the start
    //printf("DEBUG - Start of createNewAcc: User ID = %d, Name = %s\n", u.id, u.name);
    
    // Zero out the entire record structure to clear any garbage values
    memset(&r, 0, sizeof(struct Record));
    
    // Explicitly set the user ID immediately
    r.userId = u.id;
    
    // DEBUGGING: Print the userId after setting it
    //printf("DEBUG - After setting userId: r.userId = %d\n", r.userId);
    
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
    
    // DEBUGGING: Print userId again to check if it's still correct
    //printf("DEBUG - After finding last ID: r.userId = %d\n", r.userId);
    
    // Open file for appending
    FILE *pf = fopen(RECORDS, "a+");
    if (pf == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    
noAccount:
    system("clear");
    printf("\t\t\t===== New record =====\n");
    
    // DEBUGGING: Reaffirm the user ID 
    printf("Creating account for user ID: %d (Username: %s)\n", u.id, u.name);
    
    // DEBUGGING: Check if userId is still correct
    printf("DEBUG - Before input: r.userId = %d\n", r.userId);
    
    // Reset userId just to be absolutely sure
    r.userId = u.id;
    
    printf("\nEnter today's date(mm/dd/yyyy):");
    scanf("%d/%d/%d", &r.deposit.month, &r.deposit.day, &r.deposit.year);
    
    // DEBUGGING: Check if userId changed after scanf
    printf("DEBUG - After date input: r.userId = %d\n", r.userId);
    
    printf("\nEnter the account number:");
    scanf("%d", &r.accountNbr);
    
    // Reset file position to beginning for checking existing accounts
    rewind(pf);
    
    // DEBUGGING: Check if userId is still intact
    printf("DEBUG - Before checking duplicates: r.userId = %d\n", r.userId);
    
    // Check for duplicate account numbers for this user
    while (getAccountFromFile(pf, userName, &cr))
    {
        if (strcmp(userName, u.name) == 0 && cr.accountNbr == r.accountNbr)
        {
            printf("✖ This Account already exists for this user\n\n");
            goto noAccount;
        }
    }
    
    // DEBUGGING: Check if userId is still intact after checks
    //printf("DEBUG - After checking duplicates: r.userId = %d\n", r.userId);
    
    printf("\nEnter the country:");
    scanf("%s", r.country);
    printf("\nEnter the phone number:");
    scanf("%d", &r.phone);
    printf("\nEnter amount to deposit: $");
    scanf("%lf", &r.amount);
    printf("\nChoose the type of account:\n\t-> saving\n\t-> current\n\t-> fixed01(for 1 year)\n\t-> fixed02(for 2 years)\n\t-> fixed03(for 3 years)\n\n\tEnter your choice:");
    scanf("%s", r.accountType);
    
    // DEBUGGING: Final check before saving
    //printf("DEBUG - Final check before saving: r.userId = %d\n", r.userId);
    
    // Reset userId one last time
    r.userId = u.id;
    
    // Move file position to end for appending
    fseek(pf, 0, SEEK_END);
    
    // DEBUGGING: Print the exact values being written to file
    //printf("DEBUG - Writing to file: id=%d, userId=%d, name=%s, accountNbr=%d\n", 
           //r.id, r.userId, u.name, r.accountNbr);
           
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
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:%d \nAmount deposited: $%.2f \nType Of Account:%s\n",
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
