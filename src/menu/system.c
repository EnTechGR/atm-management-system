#include "../header.h"
#include "../ui/tui.h"
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "../utils/ipc_utils.h"

/* Forward: country validator lives in get_valid_country.c */
extern int findCountryByAnyField(const char *input,
                                  char *matchedCountry, size_t size);

/* ─────────────────────────────────────────────────────────────────────────── */
/* DB HELPERS                                                                  */
/* ─────────────────────────────────────────────────────────────────────────── */

int userExistsInDB(sqlite3 *db, const char *username) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT 1 FROM users WHERE name=? COLLATE NOCASE LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

int getUserIdByNameDB(sqlite3 *db, const char *username) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id FROM users WHERE name=? LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int uid = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) uid = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return uid;
}

void getAccountIDsForUserDB(sqlite3 *db, const char *username,
                             int *ids, int *count) {
    *count = 0;
    int uid = getUserIdByNameDB(db, username);
    if (uid < 0) return;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT account_id FROM accounts WHERE user_id=?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, uid);
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < 100)
        ids[(*count)++] = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* INTERNAL TUI FORM HELPERS                                                   */
/* All input is collected through ncurses windows — no tui_suspend().          */
/* Return 1 on success, 0 on ESC/cancel.                                       */
/* ─────────────────────────────────────────────────────────────────────────── */

/* Layout constants */
#define LX   3    /* label left column  */
#define FX   26   /* input field column */
#define FW   28   /* input field width  */

/* Print a right-aligned label in CP_LABEL */
static void _label(WINDOW *w, int y, const char *text) {
    wattron(w, COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvwprintw(w, y, LX, "%-*s:", FX - LX - 2, text);
    wattroff(w, COLOR_PAIR(CP_LABEL) | A_BOLD);
    wrefresh(w);
}

/* Error/status row — always the second-to-last row */
static void _err(WINDOW *w, const char *msg) {
    int ww = getmaxx(w), wh = getmaxy(w);
    wattron(w, COLOR_PAIR(CP_ERROR));
    mvwhline(w, wh - 2, 1, ' ', ww - 2);
    if (msg) mvwprintw(w, wh - 2, 2, "%s", msg);
    wattroff(w, COLOR_PAIR(CP_ERROR));
    wrefresh(w);
}
static void _err_clr(WINDOW *w) { _err(w, NULL); }

/* Restore the green border (input drawing can clobber corners) */
static void _rebox(WINDOW *w) {
    wattron(w, COLOR_PAIR(CP_BORDER));
    box(w, 0, 0);
    wattroff(w, COLOR_PAIR(CP_BORDER));
    wrefresh(w);
}

/* ── Account number ──────────────────────────────────────────────────────── */
static int _field_acct_nbr(WINDOW *w, int y,
                             int *existing, int n, int *out) {
    _label(w, y, "Account Number");
    for (;;) {
        _err_clr(w); _rebox(w);
        char buf[32] = {0};
        if (!tui_input(w, y, FX, FW, buf, 10, 0) || !buf[0]) return 0;
        int ok = 1;
        for (int i = 0; buf[i]; i++)
            if (!isdigit((unsigned char)buf[i])) { ok = 0; break; }
        if (!ok) { _err(w, "Digits only — no letters or symbols."); continue; }
        int v = atoi(buf);
        if (v <= 0) { _err(w, "Account number must be a positive integer."); continue; }
        int dup = 0;
        for (int i = 0; i < n; i++) if (existing[i] == v) { dup = 1; break; }
        if (dup) { _err(w, "That account number already exists."); continue; }
        *out = v;
        wattron(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        mvwprintw(w, y, FX + FW + 1, "[OK]");
        wattroff(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        wrefresh(w);
        return 1;
    }
}

/* ── Date ────────────────────────────────────────────────────────────────── */
static int _field_date(WINDOW *w, int y, struct Date *out) {
    time_t now = time(NULL); struct tm *t = localtime(&now);
    _label(w, y, "Date (MM/DD/YYYY)");
    wattron(w, COLOR_PAIR(CP_DIM));
    mvwprintw(w, y, FX + FW + 1, " leave blank = today");
    wattroff(w, COLOR_PAIR(CP_DIM));
    wrefresh(w);
    for (;;) {
        _err_clr(w); _rebox(w);
        char buf[20] = {0};
        tui_input(w, y, FX, FW, buf, 18, 0);
        if (!buf[0]) {          /* blank → today */
            out->month = t->tm_mon + 1;
            out->day   = t->tm_mday;
            out->year  = t->tm_year + 1900;
            wattron(w, COLOR_PAIR(CP_FIELD));
            mvwprintw(w, y, FX, "%02d/%02d/%04d", out->month, out->day, out->year);
            wattroff(w, COLOR_PAIR(CP_FIELD));
            wrefresh(w); return 1;
        }
        int mo, dy, yr;
        if (sscanf(buf, "%d/%d/%d", &mo, &dy, &yr) != 3)
            { _err(w, "Format: MM/DD/YYYY  e.g. 03/15/2024"); continue; }
        if (mo < 1 || mo > 12) { _err(w, "Month must be between 1 and 12."); continue; }
        if (yr < 2000 || yr > 2100) { _err(w, "Year must be between 2000 and 2100."); continue; }
        int md = 31;
        if      (mo == 4 || mo == 6 || mo == 9 || mo == 11) md = 30;
        else if (mo == 2)
            md = ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0) ? 29 : 28;
        if (dy < 1 || dy > md)
            { char e[56]; snprintf(e,sizeof(e),"Day must be 1-%d for month %d.",md,mo); _err(w,e); continue; }
        out->month = mo; out->day = dy; out->year = yr;
        wattron(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        mvwprintw(w, y, FX + FW + 1, "[OK]");
        wattroff(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        wrefresh(w); return 1;
    }
}

/* ── Country ──────────────────────────────────────────────────────────────── */
static int _field_country(WINDOW *w, int y, char *out) {
    _label(w, y, "Country");
    wattron(w, COLOR_PAIR(CP_DIM));
    mvwprintw(w, y, FX + FW + 1, " name / 2-letter / 3-letter code");
    wattroff(w, COLOR_PAIR(CP_DIM));
    wrefresh(w);
    for (;;) {
        _err_clr(w); _rebox(w);
        char buf[100] = {0};
        if (!tui_input(w, y, FX, FW, buf, 98, 0) || !buf[0]) return 0;
        char matched[100];
        if (findCountryByAnyField(buf, matched, sizeof(matched))) {
            strncpy(out, matched, 99); out[99] = '\0';
            wattron(w, COLOR_PAIR(CP_FIELD));
            mvwprintw(w, y, FX, "%-*s", FW, matched);
            wattroff(w, COLOR_PAIR(CP_FIELD));
            wattron(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
            mvwprintw(w, y, FX + FW + 1, "[OK]");
            wattroff(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
            wrefresh(w); return 1;
        }
        _err(w, "Country not found.  Try: 'France', 'US', 'DEU', 'Brazil' ...");
    }
}

/* ── Phone ───────────────────────────────────────────────────────────────── */
static int _field_phone(WINDOW *w, int y, char *out) {
    _label(w, y, "Phone (10 digits)");
    for (;;) {
        _err_clr(w); _rebox(w);
        char buf[20] = {0};
        if (!tui_input(w, y, FX, 12, buf, 11, 0) || !buf[0]) return 0;
        if ((int)strlen(buf) != 10) { _err(w, "Must be exactly 10 digits, no spaces or dashes."); continue; }
        int ok = 1;
        for (int i = 0; buf[i]; i++)
            if (!isdigit((unsigned char)buf[i])) { ok = 0; break; }
        if (!ok) { _err(w, "Digits only — no spaces, dashes, or letters."); continue; }
        strncpy(out, buf, 10); out[10] = '\0';
        wattron(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        mvwprintw(w, y, FX + 13, "[OK]");
        wattroff(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        wrefresh(w); return 1;
    }
}

/* ── Amount ──────────────────────────────────────────────────────────────── */
static int _field_amount(WINDOW *w, int y, double *out) {
    _label(w, y, "Amount ($)");
    for (;;) {
        _err_clr(w); _rebox(w);
        char buf[48] = {0};
        if (!tui_input(w, y, FX, FW, buf, 46, 0) || !buf[0]) return 0;
        if (strchr(buf, ',')) { _err(w, "Use '.' for decimal point, not ','."); continue; }
        char *dot = strchr(buf, '.');
        if (dot && (int)strlen(dot + 1) > 2) { _err(w, "At most 2 decimal places allowed."); continue; }
        int ok = 1, dots = 0;
        for (int i = 0; buf[i]; i++) {
            if (buf[i] == '.') { if (++dots > 1) { ok = 0; break; } }
            else if (!isdigit((unsigned char)buf[i])) { ok = 0; break; }
        }
        if (!ok) { _err(w, "Enter a valid amount, e.g.  1500  or  99.50"); continue; }
        double v = atof(buf);
        if (v <= 0.0) { _err(w, "Amount must be greater than zero."); continue; }
        *out = v;
        wattron(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        mvwprintw(w, y, FX + FW + 1, "[OK]");
        wattroff(w, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        wrefresh(w); return 1;
    }
}

/* ── Account type (menu modal) ───────────────────────────────────────────── */
static int _pick_acct_type(char *out) {
    const char *opts[] = {
        "savings   — variable,  7% interest/yr",
        "current   — no interest",
        "fixed01   — 1-year term, 4% interest/yr",
        "fixed02   — 2-year term, 5% interest/yr",
        "fixed03   — 3-year term, 8% interest/yr",
        NULL
    };
    const char *keys[] = { "savings","current","fixed01","fixed02","fixed03" };
    int ch = tui_modal_menu("ACCOUNT TYPE", opts, "Up/Down or 1-5, ENTER to select");
    if (ch < 0) return 0;
    strncpy(out, keys[ch], 9); out[9] = '\0';
    return 1;
}

/* ── Build a formatted string list for tui_pick_item ────────────────────── */
static void _build_list(int *ids, double *bals, char types[][20],
                          int count, char items[][72]) {
    for (int i = 0; i < count; i++)
        snprintf(items[i], 71, "#%-6d  $%-12.2f  %s",
                 ids[i], bals[i], types[i]);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* PUBLIC OPERATIONS                                                           */
/* ─────────────────────────────────────────────────────────────────────────── */

/* ── createNewAcc ────────────────────────────────────────────────────────── */
void createNewAcc(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    /* Load existing IDs for duplicate check */
    int existing[100], excnt = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT account_id FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && excnt < 100)
            existing[excnt++] = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    /* Form window */
    int wh = 22, ww = 70;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;
    WINDOW *win = tui_win_new(wh, ww, wy, wx, "CREATE NEW ACCOUNT");
    tui_header(u.name);
    tui_footer("ESC cancels field   ENTER accepts   Fields marked [OK] are confirmed");

    /* Info line */
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 1, LX, "New account for: %s", u.name);
    if (excnt > 0) {
        char ex[80] = "Existing IDs: ";
        for (int i = 0; i < excnt && (int)strlen(ex) < 66; i++) {
            char tmp[14];
            snprintf(tmp, sizeof(tmp), "%d%s", existing[i],
                     (i < excnt - 1) ? ", " : "");
            strncat(ex, tmp, sizeof(ex) - strlen(ex) - 1);
        }
        mvwprintw(win, 2, LX, "%s", ex);
    }
    wattroff(win, COLOR_PAIR(CP_DIM));
    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwhline(win, 3, 1, ACS_HLINE, ww - 2);
    wattroff(win, COLOR_PAIR(CP_BORDER));
    wrefresh(win);

    struct Record r; memset(&r, 0, sizeof(r)); r.userId = u.id;

    /* Collect all fields — ESC from any field aborts the whole form */
    if (!_field_acct_nbr(win,  5, existing, excnt, &r.accountNbr)) goto abort;
    if (!_field_date    (win,  7, &r.deposit))                      goto abort;
    if (!_field_country (win,  9, r.country))                       goto abort;
    if (!_field_phone   (win, 12, r.phone))                         goto abort;
    if (!_field_amount  (win, 14, &r.amount))                       goto abort;
    tui_win_destroy(win); win = NULL;
    if (!_pick_acct_type(r.accountType))                            goto abort2;

    /* Insert */
    {
        const char *ins =
            "INSERT INTO accounts "
            "(user_id,account_id,creation_date,country,phone,balance,account_type)"
            " VALUES(?,?,?,?,?,?,?);";
        int ok = 0;
        if (sqlite3_prepare_v2(db, ins, -1, &stmt, NULL) == SQLITE_OK) {
            char aid[20], date[11];
            snprintf(aid,  sizeof(aid),  "%d", r.accountNbr);
            snprintf(date, sizeof(date), "%04d-%02d-%02d",
                     r.deposit.year, r.deposit.month, r.deposit.day);
            sqlite3_bind_int   (stmt, 1, r.userId);
            sqlite3_bind_text  (stmt, 2, aid,           -1, SQLITE_STATIC);
            sqlite3_bind_text  (stmt, 3, date,           -1, SQLITE_STATIC);
            sqlite3_bind_text  (stmt, 4, r.country,      -1, SQLITE_STATIC);
            sqlite3_bind_text  (stmt, 5, r.phone,        -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 6, r.amount);
            sqlite3_bind_text  (stmt, 7, r.accountType,  -1, SQLITE_STATIC);
            ok = (sqlite3_step(stmt) == SQLITE_DONE);
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
        if (ok) tui_modal_success("Account created successfully!");
        else    tui_modal_error("Failed to create account. Please try again.");
        success(u);
        return;
    }

abort:  if (win) tui_win_destroy(win);
abort2: sqlite3_close(db);
}

/* ── updateAccount ───────────────────────────────────────────────────────── */
void updateAccount(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    int ids[100]; double bals[100]; char types[100][20]; int count = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT account_id,balance,account_type FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < 100) {
            ids[count]  = sqlite3_column_int(stmt, 0);
            bals[count] = sqlite3_column_double(stmt, 1);
            strncpy(types[count],
                    (const char *)sqlite3_column_text(stmt, 2), 19);
            types[count][19] = '\0'; count++;
        }
        sqlite3_finalize(stmt);
    }
    if (count == 0) {
        sqlite3_close(db);
        tui_modal_error("You have no accounts to update.");
        success(u); return;
    }

    /* Pick account */
    char rows[100][72]; const char *ptrs[101];
    _build_list(ids, bals, types, count, rows);
    for (int i = 0; i < count; i++) ptrs[i] = rows[i];
    ptrs[count] = NULL;
    tui_header(u.name); tui_footer("Select the account to update");
    int sel = tui_pick_item("SELECT ACCOUNT", ptrs, count, "Up/Down, ENTER");
    if (sel < 0) { sqlite3_close(db); return; }
    int acctId = ids[sel];

    /* Pick field */
    tui_footer("Choose which field to update");
    const char *fields[] = { "Country", "Phone number", NULL };
    int field = tui_modal_menu("UPDATE FIELD", fields, "ENTER to confirm");
    if (field < 0) { sqlite3_close(db); return; }

    /* Collect new value in a small window */
    int wh = 11, ww = 64;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;
    WINDOW *win = tui_win_new(wh, ww, wy, wx, "ENTER NEW VALUE");
    tui_header(u.name); tui_footer("ESC to cancel   ENTER to save");
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 1, LX, "Account #%d  —  updating: %s",
              acctId, field == 0 ? "Country" : "Phone number");
    wattroff(win, COLOR_PAIR(CP_DIM));
    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwhline(win, 2, 1, ACS_HLINE, ww - 2);
    wattroff(win, COLOR_PAIR(CP_BORDER));
    wrefresh(win);

    char newVal[100] = {0};
    int got = (field == 0) ? _field_country(win, 4, newVal)
                           : _field_phone  (win, 4, newVal);
    tui_win_destroy(win);
    if (!got) { sqlite3_close(db); return; }

    const char *sql = (field == 0)
        ? "UPDATE accounts SET country=? WHERE user_id=? AND account_id=?"
        : "UPDATE accounts SET phone=?   WHERE user_id=? AND account_id=?";
    int ok = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newVal, -1, SQLITE_STATIC);
        sqlite3_bind_int (stmt, 2, u.id);
        sqlite3_bind_int (stmt, 3, acctId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    if (ok) tui_modal_success("Account updated successfully!");
    else    tui_modal_error("Update failed. Please try again.");
    success(u);
}

/* ── checkAccountDetails ─────────────────────────────────────────────────── */
void checkAccountDetails(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    int ids[100]; double bals[100]; char types[100][20]; int count = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT account_id,balance,account_type FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < 100) {
            ids[count]  = sqlite3_column_int(stmt, 0);
            bals[count] = sqlite3_column_double(stmt, 1);
            strncpy(types[count],
                    (const char *)sqlite3_column_text(stmt, 2), 19);
            types[count][19] = '\0'; count++;
        }
        sqlite3_finalize(stmt);
    }
    if (count == 0) {
        sqlite3_close(db); tui_modal_error("You have no accounts."); success(u); return;
    }

    char rows[100][72]; const char *ptrs[101];
    _build_list(ids, bals, types, count, rows);
    for (int i = 0; i < count; i++) ptrs[i] = rows[i];
    ptrs[count] = NULL;
    tui_header(u.name); tui_footer("Select an account to view its details");
    int sel = tui_pick_item("SELECT ACCOUNT", ptrs, count, "Up/Down, ENTER");
    if (sel < 0) { sqlite3_close(db); return; }
    int acctNbr = ids[sel];

    /* Fetch full record */
    int found = 0, accId = 0; double balance = 0.0;
    char cdate[20]={0}, country[100]={0}, phone[20]={0}, atype[20]={0};
    if (sqlite3_prepare_v2(db,
        "SELECT account_id,creation_date,country,phone,balance,account_type "
        "FROM accounts WHERE user_id=? AND account_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        sqlite3_bind_int(stmt, 2, acctNbr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found   = 1;
            accId   = sqlite3_column_int(stmt, 0);
            strncpy(cdate,   (const char *)sqlite3_column_text(stmt, 1), 19);
            strncpy(country, (const char *)sqlite3_column_text(stmt, 2), 99);
            strncpy(phone,   (const char *)sqlite3_column_text(stmt, 3), 19);
            balance = sqlite3_column_double(stmt, 4);
            strncpy(atype,   (const char *)sqlite3_column_text(stmt, 5), 19);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    if (!found) { tui_modal_error("Account not found."); success(u); return; }

    int yr=0, mo=0, dy=0;
    sscanf(cdate, "%4d-%2d-%2d", &yr, &mo, &dy);

    double rate = 0.0;
    if      (!strcmp(atype,"savings")) rate = 0.07;
    else if (!strcmp(atype,"fixed01")) rate = 0.04;
    else if (!strcmp(atype,"fixed02")) rate = 0.05;
    else if (!strcmp(atype,"fixed03")) rate = 0.08;

    /* Detail window */
    int wh = 20, ww = 58;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;
    char wtitle[32]; snprintf(wtitle, sizeof(wtitle), "ACCOUNT #%d", accId);
    WINDOW *dwin = tui_win_new(wh, ww, wy, wx, wtitle);
    tui_header(u.name); tui_footer("Press any key to return to the menu");

    int y = 2;
    #define F(label, fmt, val) \
        do { wattron(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD); \
             mvwprintw(dwin, y, 3, "%-18s:", label); \
             wattroff(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD); \
             wattron(dwin, COLOR_PAIR(CP_NORMAL)|A_BOLD); \
             mvwprintw(dwin, y++, 22, fmt, val); \
             wattroff(dwin, COLOR_PAIR(CP_NORMAL)|A_BOLD); } while (0)

    F("Account Number", "%d",  accId);
    /* date row — 3 values, done manually */
    wattron(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD);
    mvwprintw(dwin, y, 3, "%-18s:", "Creation Date");
    wattroff(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD);
    wattron(dwin, COLOR_PAIR(CP_NORMAL)|A_BOLD);
    mvwprintw(dwin, y++, 22, "%02d/%02d/%04d", dy, mo, yr);
    wattroff(dwin, COLOR_PAIR(CP_NORMAL)|A_BOLD);
    F("Country",  "%s", country);
    F("Phone",    "%s", phone);
    #undef F

    /* Balance in green */
    wattron(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD);
    mvwprintw(dwin, y, 3, "%-18s:", "Balance");
    wattroff(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD);
    wattron(dwin, COLOR_PAIR(CP_SUCCESS)|A_BOLD);
    mvwprintw(dwin, y++, 22, "$%.2f", balance);
    wattroff(dwin, COLOR_PAIR(CP_SUCCESS)|A_BOLD);

    wattron(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD);
    mvwprintw(dwin, y, 3, "%-18s:", "Type");
    wattroff(dwin, COLOR_PAIR(CP_LABEL)|A_BOLD);
    wattron(dwin, COLOR_PAIR(CP_NORMAL)|A_BOLD);
    mvwprintw(dwin, y++, 22, "%s", atype);
    wattroff(dwin, COLOR_PAIR(CP_NORMAL)|A_BOLD);

    y++;
    wattron(dwin, COLOR_PAIR(CP_BORDER));
    mvwhline(dwin, y++, 2, ACS_HLINE, ww - 4);
    wattroff(dwin, COLOR_PAIR(CP_BORDER));

    wattron(dwin, COLOR_PAIR(CP_DIM));
    if (rate > 0.0) {
        double interest = balance * rate / 12.0;
        mvwprintw(dwin, y++, 3,
                  "Monthly interest: $%.2f  (%.0f%% per year)",
                  interest, rate * 100.0);
        mvwprintw(dwin, y++, 3, "Credited on day %d of each month.", dy);
    } else if (!strcmp(atype, "current")) {
        mvwprintw(dwin, y++, 3, "Current accounts do not earn interest.");
    }
    wattroff(dwin, COLOR_PAIR(CP_DIM));

    wattron(dwin, COLOR_PAIR(CP_BORDER));
    mvwprintw(dwin, wh - 2, (ww - 17) / 2, "[ Press any key ]");
    wattroff(dwin, COLOR_PAIR(CP_BORDER));
    wrefresh(dwin);
    wgetch(dwin);
    tui_win_destroy(dwin);
    success(u);
}

/* ── checkAllAccounts ────────────────────────────────────────────────────── */
void checkAllAccounts(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    int    ids[100]; double bals[100];
    char   dates[100][12], countries[100][100],
           phones[100][20], types[100][20];
    int count = 0;

    if (sqlite3_prepare_v2(db,
        "SELECT account_id,creation_date,country,phone,balance,account_type "
        "FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < 100) {
            ids[count] = sqlite3_column_int(stmt, 0);
            strncpy(dates[count],     (const char *)sqlite3_column_text(stmt,1), 11);
            strncpy(countries[count], (const char *)sqlite3_column_text(stmt,2), 99);
            strncpy(phones[count],    (const char *)sqlite3_column_text(stmt,3), 19);
            bals[count] = sqlite3_column_double(stmt, 4);
            strncpy(types[count],     (const char *)sqlite3_column_text(stmt,5), 19);
            dates[count][11]     = '\0';
            countries[count][99] = '\0';
            phones[count][19]    = '\0';
            types[count][19]     = '\0';
            count++;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    int wh = LINES - 4, ww = COLS - 4;
    char wtitle[64];
    snprintf(wtitle, sizeof(wtitle), "ALL ACCOUNTS — %s", u.name);
    WINDOW *win = tui_win_new(wh, ww, 2, 2, wtitle);
    tui_header(u.name); tui_footer("Press any key to return to the menu");

    if (count == 0) {
        wattron(win, COLOR_PAIR(CP_DIM));
        mvwprintw(win, 3, 4, "No accounts found.");
        wattroff(win, COLOR_PAIR(CP_DIM));
    } else {
        wattron(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
        mvwprintw(win, 2, 3,
                  "%-8s  %-12s  %-20s  %-12s  %-13s  %s",
                  "Acct ID", "Date", "Country", "Phone", "Balance", "Type");
        wattroff(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
        wattron(win, COLOR_PAIR(CP_BORDER));
        mvwhline(win, 3, 3, ACS_HLINE, ww - 6);
        wattroff(win, COLOR_PAIR(CP_BORDER));

        for (int i = 0; i < count && (4 + i) < wh - 2; i++) {
            int yr=0, mo=0, dy=0;
            sscanf(dates[i], "%4d-%2d-%2d", &yr, &mo, &dy);
            char dfmt[12];
            snprintf(dfmt, sizeof(dfmt), "%02d/%02d/%04d", dy, mo, yr);
            wattron(win, COLOR_PAIR(i % 2 == 0 ? CP_NORMAL : CP_DIM));
            mvwprintw(win, 4 + i, 3,
                      "%-8d  %-12s  %-20s  %-12s  $%-12.2f  %s",
                      ids[i], dfmt, countries[i], phones[i],
                      bals[i], types[i]);
            wattroff(win, COLOR_PAIR(i % 2 == 0 ? CP_NORMAL : CP_DIM));
        }
    }

    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwprintw(win, wh - 2, (ww - 17) / 2, "[ Press any key ]");
    wattroff(win, COLOR_PAIR(CP_BORDER));
    wrefresh(win);
    wgetch(win);
    tui_win_destroy(win);
    success(u);
}

/* ── makeTransaction ─────────────────────────────────────────────────────── */
void makeTransaction(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    int ids[100]; double bals[100]; char types[100][20]; int count = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT account_id,account_type,balance FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < 100) {
            ids[count]  = sqlite3_column_int(stmt, 0);
            strncpy(types[count],
                    (const char *)sqlite3_column_text(stmt, 1), 19);
            types[count][19] = '\0';
            bals[count] = sqlite3_column_double(stmt, 2);
            count++;
        }
        sqlite3_finalize(stmt);
    }
    if (count == 0) {
        sqlite3_close(db);
        tui_modal_error("No accounts available for transactions.");
        success(u); return;
    }

    /* Pick account */
    char rows[100][72]; const char *ptrs[101];
    _build_list(ids, bals, types, count, rows);
    for (int i = 0; i < count; i++) ptrs[i] = rows[i];
    ptrs[count] = NULL;
    tui_header(u.name); tui_footer("Note: fixed-term accounts cannot make transactions");
    int sel = tui_pick_item("SELECT ACCOUNT", ptrs, count, "Up/Down, ENTER");
    if (sel < 0) { sqlite3_close(db); return; }

    if (!strcmp(types[sel],"fixed01") || !strcmp(types[sel],"fixed02") ||
        !strcmp(types[sel],"fixed03")) {
        sqlite3_close(db);
        tui_modal_error("Transactions are not allowed on fixed-term accounts.");
        success(u); return;
    }

    int acctId   = ids[sel];
    double curBal = bals[sel];

    /* Transaction type */
    tui_footer("Choose transaction type");
    const char *tx_opts[] = { "Deposit", "Withdrawal", NULL };
    int txType = tui_modal_menu("TRANSACTION TYPE", tx_opts, "ENTER to confirm");
    if (txType < 0) { sqlite3_close(db); return; }

    /* Amount */
    int wh = 10, ww = 58;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;
    WINDOW *win = tui_win_new(wh, ww, wy, wx,
                               txType == 0 ? "DEPOSIT" : "WITHDRAWAL");
    tui_header(u.name); tui_footer("ESC to cancel   ENTER to confirm");
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 1, LX, "Account #%d   Current balance: $%.2f", acctId, curBal);
    wattroff(win, COLOR_PAIR(CP_DIM));
    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwhline(win, 2, 1, ACS_HLINE, ww - 2);
    wattroff(win, COLOR_PAIR(CP_BORDER));
    wrefresh(win);

    double amount = 0.0;
    if (!_field_amount(win, 4, &amount))
        { tui_win_destroy(win); sqlite3_close(db); return; }
    tui_win_destroy(win);

    if (txType == 1 && amount > curBal) {
        sqlite3_close(db);
        tui_modal_error("Insufficient funds for this withdrawal.");
        success(u); return;
    }

    double newBal = curBal + (txType == 0 ? amount : -amount);
    int ok = 0;
    if (sqlite3_prepare_v2(db,
        "UPDATE accounts SET balance=? WHERE user_id=? AND account_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, newBal);
        sqlite3_bind_int   (stmt, 2, u.id);
        sqlite3_bind_int   (stmt, 3, acctId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    if (ok) {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 txType == 0
                   ? "Deposited $%.2f   New balance: $%.2f"
                   : "Withdrew  $%.2f   New balance: $%.2f",
                 amount, newBal);
        tui_modal_success(msg);
    } else {
        tui_modal_error("Transaction failed. Please try again.");
    }
    success(u);
}

/* ── removeAccount ───────────────────────────────────────────────────────── */
void removeAccount(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    int ids[100]; double bals[100]; char types[100][20]; int count = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT account_id,balance,account_type FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < 100) {
            ids[count]  = sqlite3_column_int(stmt, 0);
            bals[count] = sqlite3_column_double(stmt, 1);
            strncpy(types[count],
                    (const char *)sqlite3_column_text(stmt, 2), 19);
            types[count][19] = '\0'; count++;
        }
        sqlite3_finalize(stmt);
    }
    if (count == 0) {
        sqlite3_close(db); tui_modal_error("No accounts to remove."); success(u); return;
    }

    char rows[100][72]; const char *ptrs[101];
    _build_list(ids, bals, types, count, rows);
    for (int i = 0; i < count; i++) ptrs[i] = rows[i];
    ptrs[count] = NULL;
    tui_header(u.name); tui_footer("Only accounts with $0.00 balance can be removed");
    int sel = tui_pick_item("SELECT ACCOUNT TO REMOVE", ptrs, count, "Up/Down, ENTER");
    if (sel < 0) { sqlite3_close(db); return; }

    if (bals[sel] > 0.0) {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "Balance is $%.2f — withdraw all funds first.", bals[sel]);
        sqlite3_close(db); tui_modal_error(msg); success(u); return;
    }

    int acctId = ids[sel];
    char qbuf[64];
    snprintf(qbuf, sizeof(qbuf), "Permanently remove account #%d?", acctId);
    if (!tui_modal_confirm(qbuf)) { sqlite3_close(db); return; }

    int ok = 0;
    if (sqlite3_prepare_v2(db,
        "DELETE FROM accounts WHERE user_id=? AND account_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        sqlite3_bind_int(stmt, 2, acctId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    if (ok) tui_modal_success("Account removed successfully.");
    else    tui_modal_error("Failed to remove account. Please try again.");
    success(u);
}

/* ── transferOwnership ───────────────────────────────────────────────────── */
void transferOwnership(struct User u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
        { tui_modal_error("Cannot open database."); return; }

    int ids[100]; double bals[100]; char types[100][20]; int count = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT account_id,balance,account_type FROM accounts WHERE user_id=?",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, u.id);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < 100) {
            ids[count]  = sqlite3_column_int(stmt, 0);
            bals[count] = sqlite3_column_double(stmt, 1);
            strncpy(types[count],
                    (const char *)sqlite3_column_text(stmt, 2), 19);
            types[count][19] = '\0'; count++;
        }
        sqlite3_finalize(stmt);
    }
    if (count == 0) {
        sqlite3_close(db);
        tui_modal_error("You have no accounts to transfer.");
        success(u); return;
    }

    char rows[100][72]; const char *ptrs[101];
    _build_list(ids, bals, types, count, rows);
    for (int i = 0; i < count; i++) ptrs[i] = rows[i];
    ptrs[count] = NULL;
    tui_header(u.name); tui_footer("Select the account you want to transfer to another user");
    int sel = tui_pick_item("SELECT ACCOUNT TO TRANSFER", ptrs, count, "Up/Down, ENTER");
    if (sel < 0) { sqlite3_close(db); return; }
    int acctId = ids[sel];

    /* Verify ownership */
    int ownerId = -1;
    if (sqlite3_prepare_v2(db,
        "SELECT user_id FROM accounts WHERE account_id=?;",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, acctId);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            ownerId = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (ownerId != u.id) {
        sqlite3_close(db); tui_modal_error("You do not own this account."); success(u); return;
    }

    /* New owner input */
    int wh = 10, ww = 56;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;
    WINDOW *win = tui_win_new(wh, ww, wy, wx, "NEW OWNER");
    tui_header(u.name); tui_footer("ESC to cancel   ENTER to submit");
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 1, LX, "Transferring account #%d", acctId);
    wattroff(win, COLOR_PAIR(CP_DIM));
    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwhline(win, 2, 1, ACS_HLINE, ww - 2);
    wattroff(win, COLOR_PAIR(CP_BORDER));
    _label(win, 4, "New owner username");
    char newOwner[50] = {0};
    int got = tui_input(win, 4, FX, 24, newOwner, 49, 0);
    tui_win_destroy(win);
    if (!got || !newOwner[0]) { sqlite3_close(db); return; }

    /* Validate */
    if (!userExistsInDB(db, newOwner)) {
        sqlite3_close(db); tui_modal_error("User not found in the system."); success(u); return;
    }
    int newIds[100]; int newCount = 0;
    getAccountIDsForUserDB(db, newOwner, newIds, &newCount);
    for (int i = 0; i < newCount; i++) {
        if (newIds[i] == acctId) {
            sqlite3_close(db);
            tui_modal_error("New owner already has an account with this ID.");
            success(u); return;
        }
    }

    int newOwnerId = getUserIdByNameDB(db, newOwner);
    if (newOwnerId < 0) {
        sqlite3_close(db); tui_modal_error("Could not retrieve new owner's ID."); success(u); return;
    }

    /* Confirm */
    char qbuf[80];
    snprintf(qbuf, sizeof(qbuf), "Transfer account #%d to '%s'?", acctId, newOwner);
    if (!tui_modal_confirm(qbuf)) { sqlite3_close(db); return; }

    int ok = 0;
    if (sqlite3_prepare_v2(db,
        "UPDATE accounts SET user_id=? WHERE account_id=?;",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, newOwnerId);
        sqlite3_bind_int(stmt, 2, acctId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    if (ok) {
        char notif[256];
        snprintf(notif, sizeof(notif),
                 "Account #%d has been transferred to you by '%s'.",
                 acctId, u.name);
        notifyUser(newOwner, notif);
        char msg[80];
        snprintf(msg, sizeof(msg), "Account #%d transferred to %s.", acctId, newOwner);
        tui_modal_success(msg);
    } else {
        tui_modal_error("Transfer failed. Please try again.");
    }
    success(u);
}