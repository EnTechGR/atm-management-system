#ifndef HEADER_H
#define HEADER_H

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
    char phone[11];
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
    char salt[33];         // 16 bytes as 32-char hex + null
    char password[65];
};

// ==== AUTHENTICATION FUNCTIONS ====
int loginMenu(char a[50], char pass[50]);
void registerMenu(char a[50], char pass[50]);
const char *getPassword(struct User u);

// ==== SYSTEM FUNCTIONS ====
void createNewAcc(struct User u);
void updateAccount(struct User u);
void checkAccountDetails(struct User u);
void makeTransaction(struct User u);
void removeAccount(struct User u);
void checkAllAccounts(struct User u);
void saveAccountToFile(FILE *ptr, struct User u, struct Record r);
void stayOrReturn(int notGood, void (*f)(struct User), struct User u); // Corrected function pointer syntax
void success(struct User u);

// ==== MENU FUNCTIONS ====
void initMenu(struct User *u);
void mainMenu(struct User u);

// ==== INPUT VALIDATION FUNCTIONS (declarations only) ====
void getValidDate(struct Date *date);
void getValidCountry(char country[100]);
void getValidPhone(char phone[11]);
void getValidAmount(double *amount);
void getValidAccountType(char accountType[10]);
int getValidAccountNumber();
int getAccountFromFile(FILE *ptr, char name[50], struct Record *r);

#endif // HEADER_H