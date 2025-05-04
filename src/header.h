#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Date
{
    int month, day, year;
};

// all fields for each record of an account
struct Record
{
    int id;
    int userId;
    char name[100];
    char country[100];
    int phone;
    char accountType[10];
    int accountNbr;
    double amount;
    struct Date deposit;
    struct Date withdraw;
};

struct User
{
    int id;
    char name[50];
    char salt[33];             // 16 bytes as 32-char hex + null
    char password[65];
};

// ==== AUTHENTICATION FUNCTIONS ====
int loginMenu(char a[50], char pass[50]);
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

