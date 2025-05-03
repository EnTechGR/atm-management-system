#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> // Include for the read() function
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
    int i = 0;
    char ch;

    system("clear");
    printf("\n\n\n\t\t\t\t  Bank Management System\n\t\t\t\t\t User Login:");
    scanf("%s", a);

    // Disable canonical mode (line buffering) and echo
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~(ECHO | ICANON); // Disable ECHO and ICANON
    nflags.c_cc[VMIN] = 1;             // Read one character at a time
    nflags.c_cc[VTIME] = 0;            // No timeout for reading

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    printf("\n\n\n\n\n\t\t\t\tEnter the password to login:");
    while (i < 49) { // Limit password length to avoid buffer overflow
        if (read(fileno(stdin), &ch, 1) != 1) {
            break; // Error reading
        }
        if (ch == '\n') {
            pass[i] = '\0';
            printf("\n");
            break;
        }
        pass[i++] = ch;
        printf("*");
    }
    pass[49] = '\0'; // Ensure null termination

    // Restore terminal
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }
}

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
            char *buff = strdup(userChecker.password); // Use strdup for safer return
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
    struct {
        int index;
        char name[50];
        char password[50];
    } users[100];
    int user_count = 0;
    char pass_retype[50];
    struct termios oflags, nflags;
    int i = 0, j = 0; // Initialize loop counters
    char ch;

    system("clear");
    printf("\n\n\n\t\t\t\t  Bank Management System\n\t\t\t\t\t User Registration:");
    printf("\n\nEnter the user name:");
    scanf("%s", a);

    if (isUsernameTaken(a)) {
        printf("\n\nUsername '%s' is already taken (case-insensitive). Please choose a different username.\n", a);
        printf("\n\nPress any key to continue...");
        getchar();
        getchar();
        return;
    }

    // Input and display asterisks for the first password
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~(ECHO | ICANON);
    nflags.c_cc[VMIN] = 1;
    nflags.c_cc[VTIME] = 0;
    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    printf("\n\nEnter the password:");
    fflush(stdout);
    while (i < 49) {
        if (read(fileno(stdin), &ch, 1) != 1) break;
        if (ch == '\n') {
            pass[i] = '\0';
            printf("\n");
            break;
        }
        pass[i++] = ch;
        printf("*");
    }
    pass[49] = '\0';

    printf("\n\nRetype the password:");
    fflush(stdout);
    while (j < 49) {
        if (read(fileno(stdin), &ch, 1) != 1) break;
        if (ch == '\n') {
            pass_retype[j] = '\0';
            printf("\n");
            break;
        }
        pass_retype[j++] = ch;
        printf("*");
    }
    pass_retype[49] = '\0';

    // Restore terminal settings
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        exit(1);
    }

    if (strcmp(pass, pass_retype) != 0) {
        printf("\n\nPasswords do not match. Registration failed.\n");
        printf("\n\nPress any key to continue...");
        getchar();
        getchar();
        return;
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
    strcpy(users[user_count].name, a);
    strcpy(users[user_count].password, pass);
    user_count++;

    // Rewrite the entire file with updated indices
    if ((write_fp = fopen(USERS, "w")) != NULL) {
        for (int k = 0; k < user_count; k++) { // Use a different loop variable
            fprintf(write_fp, "%d %s %s\n", users[k].index, users[k].name, users[k].password);
        }
        fclose(write_fp);
    } else {
        perror("Error opening users file for writing");
        return;
    }

    printf("\n\nUser %s registered successfully!\n", a);
    printf("\n\nPress any key to continue...");
    getchar();
    getchar();
}