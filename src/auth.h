// auth.h
#ifndef AUTH_H
#define AUTH_H

#include "header.h" // Assuming User struct is defined here

int loginMenu(char a[50], char pass[50]);
const char *getPassword(struct User u);
int isUsernameTaken(const char *username);
void registerMenu(char a[50], char pass[50]);

#endif // AUTH_H