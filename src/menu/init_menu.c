#include "../header.h"
#include "../ui/tui.h"
#include <sqlite3.h>
#include <openssl/sha.h>
#include <ctype.h>
#include <string.h>

/* ── Forward declarations for auth helpers (auth.c) ─────────────────────── */
extern const char *getPassword(struct User u);
extern int  verifyPassword(const char *password, const char *stored);
extern int  isUsernameTaken(const char *username);
extern void generateSalt(unsigned char *salt, size_t length);
extern void hashPassword(const char *pw, const unsigned char *salt,
                         size_t salt_len, unsigned char *hash);
extern void bin2hex(unsigned char *bin, size_t bin_len, char *hex);

/* ── DB: look up user by name ────────────────────────────────────────────── */
void getUserById(char username[50], struct User *u) {
    sqlite3 *db; sqlite3_stmt *stmt;
    u->id = -1;
    strncpy(u->name, username, sizeof(u->name) - 1);
    u->name[sizeof(u->name) - 1] = '\0';
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK) return;
    const char *sql = "SELECT id FROM users WHERE LOWER(name)=LOWER(?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            u->id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Login screen                                                                */
/* ─────────────────────────────────────────────────────────────────────────── */
static int _login_screen(char *username, char *password) {
    int wh = 14, ww = 52;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;

    clear();
    tui_header(NULL);
    tui_footer("ENTER to submit   ESC to cancel");

    WINDOW *win = tui_win_new(wh, ww, wy, wx, "LOGIN");

    wattron(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvwprintw(win,  3, 4, "%-18s:", "Username");
    mvwprintw(win,  6, 4, "%-18s:", "Password");
    wattroff(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, wh - 2, 3, "Letters a-z only  |  Password 8-12 chars");
    wattroff(win, COLOR_PAIR(CP_DIM));
    wrefresh(win);

    char uname[50] = {0}, pword[50] = {0};

    tui_input(win, 3, 23, 26, uname, 49, 0);
    if (!uname[0]) { tui_win_destroy(win); return 0; }

    /* Validate username: letters only */
    for (int i = 0; uname[i]; i++) {
        if (!isalpha((unsigned char)uname[i])) {
            tui_win_destroy(win);
            tui_modal_error("Username: letters a-z / A-Z only.");
            return 0;
        }
    }

    tui_input(win, 6, 23, 26, pword, 49, 1);
    tui_win_destroy(win);
    if (!pword[0]) return 0;

    int plen = (int)strlen(pword);
    if (plen < 8 || plen > 12) {
        tui_modal_error("Password must be 8-12 characters.");
        return 0;
    }

    /* Verify credentials */
    struct User tmp; memset(&tmp, 0, sizeof(tmp));
    strncpy(tmp.name, uname, sizeof(tmp.name) - 1);
    const char *stored = getPassword(tmp);
    if (strcmp(stored, "no user found") == 0) {
        tui_modal_error("User not found. Please register first.");
        return 0;
    }
    if (!verifyPassword(pword, stored)) {
        tui_modal_error("Invalid password. Access denied.");
        return 0;
    }

    strncpy(username, uname,  49); username[49]  = '\0';
    strncpy(password, pword,  49); password[49]  = '\0';
    tui_modal_success("Login successful!");
    return 1;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Register screen                                                             */
/* ─────────────────────────────────────────────────────────────────────────── */
static void _register_screen(void) {
    int wh = 18, ww = 56;
    int wy = (LINES - wh) / 2, wx = (COLS - ww) / 2;

    clear();
    tui_header(NULL);
    tui_footer("ENTER to submit   ESC to cancel");

    WINDOW *win = tui_win_new(wh, ww, wy, wx, "REGISTER");

    wattron(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvwprintw(win,  3, 4, "%-18s:", "Username");
    mvwprintw(win,  6, 4, "%-18s:", "Password");
    mvwprintw(win,  9, 4, "%-18s:", "Confirm Password");
    wattroff(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, wh - 3, 3, "Username: letters only");
    mvwprintw(win, wh - 2, 3, "Password: 8-12 alphanumeric characters");
    wattroff(win, COLOR_PAIR(CP_DIM));
    wrefresh(win);

    char uname[50] = {0}, pword[50] = {0}, confirm[50] = {0};

    tui_input(win,  3, 23, 28, uname,   49, 0);
    if (!uname[0])   { tui_win_destroy(win); return; }

    tui_input(win,  6, 23, 28, pword,   49, 1);
    if (!pword[0])   { tui_win_destroy(win); return; }

    tui_input(win,  9, 23, 28, confirm, 49, 1);
    tui_win_destroy(win);
    if (!confirm[0]) return;

    /* ── Validation ── */
    for (int i = 0; uname[i]; i++) {
        if (!isalpha((unsigned char)uname[i])) {
            tui_modal_error("Username: letters a-z / A-Z only.");
            return;
        }
    }
    if (!uname[0]) { tui_modal_error("Username cannot be empty."); return; }

    if (strcmp(pword, confirm) != 0) {
        tui_modal_error("Passwords do not match.");
        return;
    }
    int plen = (int)strlen(pword);
    if (plen < 8 || plen > 12) {
        tui_modal_error("Password must be 8-12 characters.");
        return;
    }
    for (int i = 0; pword[i]; i++) {
        if (!isalnum((unsigned char)pword[i])) {
            tui_modal_error("Password: letters and digits only.");
            return;
        }
    }
    if (strchr(pword, ' ')) {
        tui_modal_error("Password cannot contain spaces.");
        return;
    }
    if (isUsernameTaken(uname)) {
        tui_modal_error("Username is already taken.");
        return;
    }

    /* ── Hash & store ── */
    unsigned char salt[16], hash[SHA256_DIGEST_LENGTH];
    char salt_hex[33], hash_hex[65], combined[130];

    generateSalt(salt, sizeof(salt));
    hashPassword(pword, salt, sizeof(salt), hash);
    bin2hex(salt, sizeof(salt), salt_hex);
    bin2hex(hash, sizeof(hash), hash_hex);
    snprintf(combined, sizeof(combined), "%s$%s", salt_hex, hash_hex);

    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK) {
        tui_modal_error("Database error. Please try again.");
        return;
    }
    const char *sql = "INSERT INTO users (name, password) VALUES (?, ?)";
    int saved = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, uname,    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, combined, -1, SQLITE_TRANSIENT);
        saved = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    if (saved)
        tui_modal_success("Registration successful!  Please log in.");
    else
        tui_modal_error("Failed to save user. Please try again.");
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* initMenu — welcome screen + login / register dispatcher                    */
/* ─────────────────────────────────────────────────────────────────────────── */
void initMenu(struct User *u) {
    char username[50], password[50];

    const char *opts[] = { "Login", "Register", "Exit", NULL };

    while (1) {
        clear();
        tui_header(NULL);
        tui_footer("Up/Down or 1-3 to navigate   ENTER to select");

        /* Logo centred in the upper section */
        int logo_w = 20;   /* visual width of the ASCII art lines */
        int logo_h = 16;    /* 5 ASCII lines + subtitle on the next line */
        int logo_y = (LINES - logo_h - 10) / 2;
        if (logo_y < 2) logo_y = 2;
        int logo_x = (COLS - logo_w) / 2;
        tui_draw_logo(stdscr, logo_y, logo_x);

        /* Tagline */
        const char *tag = "Secure  |  Reliable  |  Fast";
        wattron(stdscr, COLOR_PAIR(CP_DIM));
        mvprintw(logo_y + logo_h + 1,
                 (COLS - (int)strlen(tag)) / 2, "%s", tag);
        wattroff(stdscr, COLOR_PAIR(CP_DIM));

        /* Separator */
        wattron(stdscr, COLOR_PAIR(CP_BORDER));
        mvhline(logo_y + logo_h + 3, (COLS - 44) / 2, ACS_HLINE, 44);
        wattroff(stdscr, COLOR_PAIR(CP_BORDER));

        refresh();

        int choice = tui_modal_menu("WELCOME", opts,
                                     "Up/Down arrows or 1-3, then ENTER");
        switch (choice) {
            case 0:
                if (_login_screen(username, password)) {
                    getUserById(username, u);
                    return;
                }
                break;
            case 1:
                _register_screen();
                break;
            case 2:
            case -1:
                tui_cleanup();
                exit(0);
        }
    }
}