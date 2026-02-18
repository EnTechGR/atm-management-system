#include "validators.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Internal: forward-declare the country lookup from get_valid_country.c ── */
extern int findCountryByAnyField(const char *input,
                                  char *matchedCountry, size_t size);

/* ── val_account_number ──────────────────────────────────────────────────── */
int val_account_number(const char *input,
                       const int  *existing, int count,
                       int *out_value,
                       char err_out[VALIDATOR_ERR_BUF])
{
    if (!input || !input[0]) {
        snprintf(err_out, VALIDATOR_ERR_BUF, "Account number cannot be empty.");
        return 0;
    }
    size_t len = strlen(input);
    if (len > 10) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Account number too long (max 10 digits).");
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)input[i])) {
            snprintf(err_out, VALIDATOR_ERR_BUF,
                     "Digits only — no letters or symbols.");
            return 0;
        }
    }
    int v = atoi(input);
    if (v <= 0) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Account number must be a positive integer.");
        return 0;
    }
    for (int i = 0; i < count; i++) {
        if (existing[i] == v) {
            snprintf(err_out, VALIDATOR_ERR_BUF,
                     "Account #%d already exists for this user.", v);
            return 0;
        }
    }
    if (out_value) *out_value = v;
    return 1;
}

/* ── val_date ────────────────────────────────────────────────────────────── */
int val_date(const char *input,
             int *m, int *d, int *y,
             char err_out[VALIDATOR_ERR_BUF])
{
    if (!input || !input[0]) {
        snprintf(err_out, VALIDATOR_ERR_BUF, "Date cannot be empty.");
        return 0;
    }
    int mo, dy, yr;
    if (sscanf(input, "%d/%d/%d", &mo, &dy, &yr) != 3) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Format must be MM/DD/YYYY, e.g. 03/15/2024.");
        return 0;
    }
    if (mo < 1 || mo > 12) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Month must be between 1 and 12.");
        return 0;
    }
    if (yr < 2000 || yr > 2100) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Year must be between 2000 and 2100.");
        return 0;
    }
    int md = 31;
    if      (mo == 4 || mo == 6 || mo == 9 || mo == 11) md = 30;
    else if (mo == 2)
        md = ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0) ? 29 : 28;
    if (dy < 1 || dy > md) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Day must be between 1 and %d for month %d.", md, mo);
        return 0;
    }
    if (m) *m = mo;
    if (d) *d = dy;
    if (y) *y = yr;
    return 1;
}

/* ── val_country ─────────────────────────────────────────────────────────── */
int val_country(const char *input,
                char matched_out[100],
                char err_out[VALIDATOR_ERR_BUF])
{
    if (!input || !input[0]) {
        snprintf(err_out, VALIDATOR_ERR_BUF, "Country cannot be empty.");
        return 0;
    }
    size_t len = strlen(input);
    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)input[i]) &&
            input[i] != ' ' && input[i] != '-') {
            snprintf(err_out, VALIDATOR_ERR_BUF,
                     "Use letters, digits, spaces or hyphens only.");
            return 0;
        }
    }
    if (!findCountryByAnyField(input, matched_out, 100)) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Not found. Try: 'France', 'US', 'DEU', 'Brazil'...");
        return 0;
    }
    return 1;
}

/* ── val_phone ───────────────────────────────────────────────────────────── */
int val_phone(const char *input,
              char err_out[VALIDATOR_ERR_BUF])
{
    if (!input || !input[0]) {
        snprintf(err_out, VALIDATOR_ERR_BUF, "Phone number cannot be empty.");
        return 0;
    }
    int len = (int)strlen(input);
    if (len < 8 || len > 10) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Phone must be 8 to 10 digits (you entered %d).", len);
        return 0;
    }
    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char)input[i])) {
            snprintf(err_out, VALIDATOR_ERR_BUF,
                     "Digits only — no spaces, dashes, or letters.");
            return 0;
        }
    }
    return 1;
}

/* ── val_amount ──────────────────────────────────────────────────────────── */
int val_amount(const char *input,
               double *out_value,
               char err_out[VALIDATOR_ERR_BUF])
{
    if (!input || !input[0]) {
        snprintf(err_out, VALIDATOR_ERR_BUF, "Amount cannot be empty.");
        return 0;
    }
    if (strchr(input, ',')) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Use '.' as decimal point, not ','.");
        return 0;
    }
    int dots = 0;
    for (int i = 0; input[i]; i++) {
        if (input[i] == '.') {
            if (++dots > 1) {
                snprintf(err_out, VALIDATOR_ERR_BUF,
                         "Too many decimal points.");
                return 0;
            }
        } else if (!isdigit((unsigned char)input[i])) {
            snprintf(err_out, VALIDATOR_ERR_BUF,
                     "Enter a valid amount, e.g. 1500 or 99.50.");
            return 0;
        }
    }
    const char *dot = strchr(input, '.');
    if (dot && (int)strlen(dot + 1) > 2) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "At most 2 decimal places allowed.");
        return 0;
    }
    char *end;
    double v = strtod(input, &end);
    if (*end != '\0' || v <= 0.0) {
        snprintf(err_out, VALIDATOR_ERR_BUF,
                 "Amount must be greater than zero.");
        return 0;
    }
    if (out_value) *out_value = v;
    return 1;
}

/* ── val_account_type ────────────────────────────────────────────────────── */
int val_account_type(const char *input,
                     char err_out[VALIDATOR_ERR_BUF])
{
    static const char *valid[] =
        { "savings", "current", "fixed01", "fixed02", "fixed03" };
    for (int i = 0; i < 5; i++)
        if (strcmp(input, valid[i]) == 0) return 1;
    snprintf(err_out, VALIDATOR_ERR_BUF,
             "Type must be: savings, current, fixed01, fixed02, or fixed03.");
    return 0;
}