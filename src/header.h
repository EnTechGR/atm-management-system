#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Date
{
    int month;
    int day;
    int year;
};

// all fields for each record of an account
struct Record
{
    int id;
    int userId;
    char name[100]; // User's name associated with this record
    int accountNbr;
    struct Date deposit;
    char country[100];
    char phone[20]; // Corrected to char array for phone number
    double amount;
    char accountType[10];
    // struct Date withdraw; // Uncomment if you plan to store withdrawal dates
};

struct User
{
    int id;
    char name[50];
    char salt[33];             // 16 bytes as 32-char hex + null
    char hashedPassword[65];
};

// ==== AUTHENTICATION FUNCTIONS ====
void loginMenu(char a[50], char pass[50]);
void registerMenu(char a[50], char pass[50]);
const char *getPassword(struct User u);

// ==== SYSTEM FUNCTIONS ====
void createNewAcc(struct User u);
void checkAllAccounts(struct User u);
void saveAccountToFile(FILE *ptr, struct User u, struct Record r);
void stayOrReturn(int notGood, void f(struct User u), struct User u);
void success(struct User u);

// ==== MENU FUNCTIONS ====
void initMenu(struct User *u);
void mainMenu(struct User u);
void updateAccountInfo(struct User loggedInUser);

