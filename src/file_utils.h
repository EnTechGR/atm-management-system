#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "header.h"

int getAccountFromFile(FILE *ptr, char name[50], struct Record *r);
void saveAccountToFile(FILE *ptr, struct User u, struct Record r);

#endif
