#include <time.h>
#include "../header.h"

// Get today's date and validate user input for date
#include <time.h>
#include "../header.h"
#include "validators.h"

void getValidDate(struct Date *date) {
    time_t now; struct tm *ct;
    char input[20], err[VALIDATOR_ERR_BUF];

    time(&now); ct = localtime(&now);
    int def_m = ct->tm_mon + 1, def_d = ct->tm_mday, def_y = ct->tm_year + 1900;

    while (1) {
        printf("\nEnter date (MM/DD/YYYY) or press Enter for today [%02d/%02d/%04d]: ",
               def_m, def_d, def_y);
        if (fgets(input, sizeof(input), stdin) == NULL) continue;
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[--len] = '\0';

        if (len == 0) {
            date->month = def_m; date->day = def_d; date->year = def_y;
            return;
        }
        int m, d, y;
        if (val_date(input, &m, &d, &y, err)) {
            date->month = m; date->day = d; date->year = y;
            return;
        }
        printf("Error: %s\n", err);
    }
}
