#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "header.h" // Assuming header.h contains the definition for struct User

char *USERS = "./data/users.txt";

// Helper function to convert a string to lowercase
void toLower(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void loginMenu(char a[50], char pass[50]) {
    struct termios oflags, nflags;

    system("clear");
    printf("\n\n\n\t\t\t\t  Bank Management System\n\t\t\t\t\t User Login:");
    scanf("%s", a);

    // disabling echo
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
    printf("\n\n\n\n\n\t\t\t\tEnter the password to login:");
    scanf("%s", pass);

    // restore terminal
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
};

const char *getPassword(struct User u) {
    FILE *fp;
    struct User userChecker;

    if ((fp = fopen("./data/users.txt", "r")) == NULL) {
        printf("Error! opening file");
        exit(1);
    }

    while (fscanf(fp, "%*d %s %s", userChecker.name, userChecker.password) != EOF) {
        if (strcmp(userChecker.name, u.name) == 0) {
            fclose(fp);
            char *buff = userChecker.password;
            return buff;
        }
    }

    fclose(fp);
    return "no user found";
}

int isUsernameTaken(const char *username) {
    FILE *fp;
    struct User userChecker;
    char lowerInputUsername[50];
    char lowerFileUsername[50];

    strncpy(lowerInputUsername, username, sizeof(lowerInputUsername) - 1);
    lowerInputUsername[sizeof(lowerInputUsername) - 1] = '\0';
    toLower(lowerInputUsername);

    if ((fp = fopen(USERS, "r")) == NULL) {
        printf("Error! opening file");
        exit(1);
    }

    while (fscanf(fp, "%*d %s %s", userChecker.name, userChecker.password) != EOF) {
        strncpy(lowerFileUsername, userChecker.name, sizeof(lowerFileUsername) - 1);
        lowerFileUsername[sizeof(lowerFileUsername) - 1] = '\0';
        toLower(lowerFileUsername);

        if (strcmp(lowerInputUsername, lowerFileUsername) == 0) {
            fclose(fp);
            return 1; // Username found (case-insensitive), it's taken
        }
    }

    fclose(fp);
    return 0; // Username not found
}

void registerMenu(char a[50], char pass[50]) {
    FILE *read_fp = NULL;
    FILE *write_fp = NULL;
    struct termios oflags, nflags;
    struct {
        int index;
        char name[50];
        char password[50];
    } users[100];
    int user_count = 0;
    char sanitized_name[50];
    int i, j;
    system("clear");
    printf("\n\n\n\t\t\t\t  Bank Management System\n\t\t\t\t\t User Registration:");
    printf("\n\nEnter the user name:");
    fgets(a, 50, stdin);
    a[strcspn(a, "\n")] = 0;
    // Sanitize the username input
    // Check for spaces in username
    for (i = 0; a[i] != '\0'; i++) {
        if (a[i] == ' ') {
            printf("\n\nInvalid username. Spaces are not allowed in usernames.\n");
            printf("\n\nPress any key to continue...");
            getchar();
            return;
        }
    }
    
    // Sanitize username (removing other invalid characters)
    j = 0;
    for (i = 0; a[i] != '\0'; i++) {
        if (isalnum(a[i]) || a[i] == '_' || a[i] == '-' || a[i] == '.') {
            if ((size_t)j < sizeof(sanitized_name) - 1) {
                sanitized_name[j++] = a[i];
            }
        }
    }
    sanitized_name[j] = '\0';
    if (strlen(sanitized_name) == 0) {
        printf("\n\nInvalid username. Please use alphanumeric characters, underscores, hyphens, or dots only.\n");
        printf("\n\nPress any key to continue...");
        //while (getchar() != '\n' && getchar() != EOF);
        return;
    }
    if (isUsernameTaken(sanitized_name)) {
        printf("\n\nUsername '%s' is already taken (case-insensitive). Please choose a different username.\n", sanitized_name);
        printf("\n\nPress any key to continue...");
        //while (getchar() != '\n' && getchar() != EOF);
        getchar();
        return;
    }
    // Disable echo for password input
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;
    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
    printf("\n\nEnter the password:");
    fgets(pass, 50, stdin);
    pass[strcspn(pass, "\n")] = 0;
    
    // REMOVED: This was clearing the input buffer after reading the password,
    // causing the need to press Enter twice
    // while (getchar() != '\n' && getchar() != EOF);
    
    // Restore terminal settings
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
    // Read all existing users
    if ((read_fp = fopen(USERS, "r")) != NULL) {
        while (fscanf(read_fp, "%d %s %s", &users[user_count].index, users[user_count].name, users[user_count].password) == 3) {
            user_count++;
        }
        fclose(read_fp);
    }
    // Add the new user to the array
    users[user_count].index = user_count;
    strcpy(users[user_count].name, sanitized_name);
    strcpy(users[user_count].password, pass);
    user_count++;
    // Rewrite the entire file with updated indices
    if ((write_fp = fopen(USERS, "w")) != NULL) {
        for (int i = 0; i < user_count; i++) {
            fprintf(write_fp, "%d %s %s\n", users[i].index, users[i].name, users[i].password);
        }
        fclose(write_fp);
    } else {
        perror("Error opening users file for writing");
        return;
    }
    printf("\n\nUser %s registered successfully!\n", sanitized_name);
    printf("\n\nPress any key to continue...");
    getchar(); // This is needed to wait for user input before continuing
}