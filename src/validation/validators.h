#ifndef VALIDATORS_H
#define VALIDATORS_H

/* ── Pure validation helpers ──────────────────────────────────────────────
   No I/O, no ncurses, no printf.
   Each function returns 1 (valid) or 0 (invalid) and writes a human-
   readable reason into `err_out` (size ERR_BUF) when it returns 0.
   ─────────────────────────────────────────────────────────────────────── */

#define VALIDATOR_ERR_BUF 128   /* size callers must pass for err_out */

#include <stddef.h>

/* Account number ──────────────────────────────────────────────────────── */
/* existing[] is the array of already-used IDs, count is its length.      */
int val_account_number(const char *input,
                       const int  *existing, int count,
                       int *out_value,
                       char err_out[VALIDATOR_ERR_BUF]);

/* Date ────────────────────────────────────────────────────────────────── */
/* Accepts "MM/DD/YYYY".  Fills *m, *d, *y on success.                   */
int val_date(const char *input,
             int *m, int *d, int *y,
             char err_out[VALIDATOR_ERR_BUF]);

/* Country ─────────────────────────────────────────────────────────────── */
/* Looks up input in countries.txt; writes canonical name to matched_out. */
int val_country(const char *input,
                char matched_out[100],
                char err_out[VALIDATOR_ERR_BUF]);

/* Phone ───────────────────────────────────────────────────────────────── */
/* Accepts 8–10 digit strings.                                            */
int val_phone(const char *input,
              char err_out[VALIDATOR_ERR_BUF]);

/* Amount ──────────────────────────────────────────────────────────────── */
/* Positive decimal, at most 2 decimal places, no commas.                 */
int val_amount(const char *input,
               double *out_value,
               char err_out[VALIDATOR_ERR_BUF]);

/* Account type ────────────────────────────────────────────────────────── */
/* input must be one of: savings current fixed01 fixed02 fixed03          */
int val_account_type(const char *input,
                     char err_out[VALIDATOR_ERR_BUF]);

#endif /* VALIDATORS_H */