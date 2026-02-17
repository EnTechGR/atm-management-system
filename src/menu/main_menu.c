#include "../header.h"
#include "../ui/tui.h"
#include <sqlite3.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* ── DB: quick account summary for the info panel ───────────────────────── */
static void _summary(int user_id, int *count, double *total) {
    *count = 0; *total = 0.0;
    sqlite3 *db; sqlite3_stmt *stmt;
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK) return;
    const char *sql =
        "SELECT COUNT(*), COALESCE(SUM(balance),0) FROM accounts WHERE user_id=?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *count = sqlite3_column_int   (stmt, 0);
            *total = sqlite3_column_double(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
}

/* ── Right-side info panel ───────────────────────────────────────────────── */
static void _draw_info(WINDOW *win, struct User u, int acct_count, double total) {
    werase(win);
    int ww = getmaxx(win);

    wattron(win, COLOR_PAIR(CP_BORDER));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(CP_BORDER));

    wattron(win, COLOR_PAIR(CP_TITLE) | A_BOLD);
    const char *hdr = " OVERVIEW ";
    mvwprintw(win, 0, (ww - (int)strlen(hdr)) / 2, "%s", hdr);
    wattroff(win, COLOR_PAIR(CP_TITLE) | A_BOLD);

    int y = 2;
    wattron(win, COLOR_PAIR(CP_LABEL));
    mvwprintw(win, y++, 3, "Welcome back,");
    wattroff(win, COLOR_PAIR(CP_LABEL));
    wattron(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvwprintw(win, y++, 4, "%s", u.name);
    wattroff(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);

    y++;
    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwhline(win, y++, 2, ACS_HLINE, ww - 4);
    wattroff(win, COLOR_PAIR(CP_BORDER));

    wattron(win, COLOR_PAIR(CP_LABEL));
    mvwprintw(win, y, 3, "Accounts     :");
    wattroff(win, COLOR_PAIR(CP_LABEL));
    wattron(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvwprintw(win, y++, 18, "%d", acct_count);
    wattroff(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);

    wattron(win, COLOR_PAIR(CP_LABEL));
    mvwprintw(win, y, 3, "Total Balance:");
    wattroff(win, COLOR_PAIR(CP_LABEL));
    wattron(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    mvwprintw(win, y++, 18, "$%.2f", total);
    wattroff(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);

    y++;
    wattron(win, COLOR_PAIR(CP_BORDER));
    mvwhline(win, y++, 2, ACS_HLINE, ww - 4);
    wattroff(win, COLOR_PAIR(CP_BORDER));

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char datebuf[32];
    strftime(datebuf, sizeof(datebuf), "%A, %d %b %Y", tm_info);
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, y++, 3, "%s", datebuf);
    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M", tm_info);
    mvwprintw(win, y++, 3, "%s", timebuf);
    wattroff(win, COLOR_PAIR(CP_DIM));

    wrefresh(win);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* mainMenu                                                                    */
/* ─────────────────────────────────────────────────────────────────────────── */
void mainMenu(struct User u) {
    const char *item_names[] = {
        "1  Create New Account",
        "2  Update Account Info",
        "3  Check Account Details",
        "4  View All Accounts",
        "5  Make a Transaction",
        "6  Remove an Account",
        "7  Transfer Ownership",
        "8  Exit",
        NULL
    };
    int n_items = 8;

    while (1) {
        clear();
        tui_header(u.name);
        tui_footer("Up/Down or 1-8 to navigate   ENTER to select   Q to quit");

        int acct_count; double total;
        _summary(u.id, &acct_count, &total);

        int panel_y  = 1;
        int total_h  = LINES - 2;
        int menu_w   = 34;
        int info_w   = COLS - menu_w - 3;
        if (info_w < 24) info_w = 24;
        if (total_h < 12) total_h = 12;

        /* Build ncurses MENU */
        ITEM **items = calloc((size_t)(n_items + 1), sizeof(ITEM *));
        for (int i = 0; i < n_items; i++)
            items[i] = new_item(item_names[i], "");
        items[n_items] = NULL;

        MENU   *menu       = new_menu(items);
        WINDOW *menu_outer = tui_win_new(total_h, menu_w, panel_y, 1, "MAIN MENU");
        WINDOW *menu_inner = derwin(menu_outer, total_h - 2, menu_w - 4, 1, 2);

        set_menu_win (menu, menu_outer);
        set_menu_sub (menu, menu_inner);
        set_menu_mark(menu, " > ");          /* pure ASCII — no width issues */
        set_menu_fore(menu, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        set_menu_back(menu, COLOR_PAIR(CP_NORMAL));
        set_menu_grey(menu, COLOR_PAIR(CP_DIM));
        menu_opts_off(menu, O_SHOWDESC);
        post_menu(menu);
        wrefresh(menu_outer);

        /* Info panel */
        WINDOW *info_win = newwin(total_h, info_w, panel_y, menu_w + 2);
        _draw_info(info_win, u, acct_count, total);

        /* Input loop */
        keypad(menu_outer, TRUE);
        int ch, chosen = -1;
        while ((ch = wgetch(menu_outer)) != ERR) {
            if      (ch == KEY_DOWN || ch == 'j') menu_driver(menu, REQ_DOWN_ITEM);
            else if (ch == KEY_UP   || ch == 'k') menu_driver(menu, REQ_UP_ITEM);
            else if (ch == '\n' || ch == KEY_ENTER) {
                chosen = item_index(current_item(menu)); break;
            }
            else if (ch == 'q' || ch == 'Q') { chosen = n_items - 1; break; }
            else if (ch >= '1' && ch <= '8') { chosen = ch - '1'; break; }
            wrefresh(menu_outer);
        }

        /* Tear down MENU widgets */
        unpost_menu(menu);
        free_menu(menu);
        for (int i = 0; i < n_items; i++) free_item(items[i]);
        free(items);
        werase(menu_inner); wrefresh(menu_inner); delwin(menu_inner);
        werase(menu_outer); wrefresh(menu_outer); delwin(menu_outer);
        werase(info_win);   wrefresh(info_win);   delwin(info_win);

        if (chosen < 0) continue;

        switch (chosen) {
            case 0: createNewAcc(u);        break;
            case 1: updateAccount(u);       break;
            case 2: checkAccountDetails(u); break;
            case 3: checkAllAccounts(u);    break;
            case 4: makeTransaction(u);     break;
            case 5: removeAccount(u);       break;
            case 6: transferOwnership(u);   break;
            case 7: tui_cleanup(); exit(0); break;
        }
        /* After each operation the loop continues and the menu redraws cleanly */
    }
}