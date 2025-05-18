#include "file_utils.h"

int getAccountFromFile(FILE *ptr, char name[50], struct Record *r) {
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

void saveAccountToFile(FILE *ptr, struct User u, struct Record r) {
    fprintf(ptr, "%d %d %s %d %d/%d/%d %s %s %.2lf %s\n\n",
            r.id,
            u.id,
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
