#include "tui.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ─────────────────────────────────────────────────────────────────────────── */
/* Internal: UTF-8 display-width counter                                       */
/* ─────────────────────────────────────────────────────────────────────────── */
int tui_dispw(const char *s) {
    int w = 0;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        unsigned long cp = 0;
        int bytes = 1;
        if      (*p < 0x80)                               { cp = *p;            bytes = 1; }
        else if ((*p & 0xE0) == 0xC0 && *(p+1))          { cp = (*p & 0x1F) << 6  | (*(p+1) & 0x3F);                            bytes = 2; }
        else if ((*p & 0xF0) == 0xE0 && *(p+1) && *(p+2)){ cp = (*p & 0x0F) << 12 | (*(p+1) & 0x3F) << 6 | (*(p+2) & 0x3F);    bytes = 3; }
        else if ((*p & 0xF8) == 0xF0 && *(p+1) && *(p+2) && *(p+3)) {
            cp = (*p & 0x07) << 18 | (*(p+1) & 0x3F) << 12 | (*(p+2) & 0x3F) << 6 | (*(p+3) & 0x3F);
            bytes = 4;
        }
        /* Emoji / wide codepoints (U+1F000 and above) occupy 2 columns */
        w += (cp >= 0x1F000) ? 2 : 1;
        p += bytes;
    }
    return w;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Color initialisation                                                        */
/* ─────────────────────────────────────────────────────────────────────────── */
static void _colors_256(void) {
    /* Gold   ~ RGB(210,170,50)  scaled to 0-1000 */
    init_color(CLR_GOLD,     820, 667, 196);
    /* Dim gold ~ RGB(140,110,30) */
    init_color(CLR_GOLD_DIM, 549, 431, 118);
    /* Dark green ~ RGB(0,130,70) */
    init_color(CLR_DKGREEN,  0,   510, 275);
    /* Near-black bg ~ RGB(18,18,18) */
    init_color(CLR_DKBG,     71,  71,  71);

    init_pair(CP_NORMAL,   CLR_GOLD,     COLOR_BLACK);
    init_pair(CP_BORDER,   CLR_DKGREEN,  COLOR_BLACK);
    init_pair(CP_SELECTED, COLOR_BLACK,  CLR_GOLD);
    init_pair(CP_TITLE,    CLR_GOLD,     COLOR_BLACK);
    init_pair(CP_LABEL,    CLR_DKGREEN,  COLOR_BLACK);
    init_pair(CP_FIELD,    COLOR_WHITE,  CLR_DKBG);
    init_pair(CP_ERROR,    COLOR_RED,    COLOR_BLACK);
    init_pair(CP_SUCCESS,  COLOR_GREEN,  COLOR_BLACK);
    init_pair(CP_STATUS,   COLOR_BLACK,  CLR_DKGREEN);
    init_pair(CP_DIM,      CLR_GOLD_DIM, COLOR_BLACK);
}

static void _colors_8(void) {
    init_pair(CP_NORMAL,   COLOR_YELLOW, COLOR_BLACK);
    init_pair(CP_BORDER,   COLOR_GREEN,  COLOR_BLACK);
    init_pair(CP_SELECTED, COLOR_BLACK,  COLOR_YELLOW);
    init_pair(CP_TITLE,    COLOR_YELLOW, COLOR_BLACK);
    init_pair(CP_LABEL,    COLOR_GREEN,  COLOR_BLACK);
    init_pair(CP_FIELD,    COLOR_WHITE,  COLOR_BLACK);
    init_pair(CP_ERROR,    COLOR_RED,    COLOR_BLACK);
    init_pair(CP_SUCCESS,  COLOR_GREEN,  COLOR_BLACK);
    init_pair(CP_STATUS,   COLOR_BLACK,  COLOR_GREEN);
    init_pair(CP_DIM,      COLOR_YELLOW, COLOR_BLACK);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Lifecycle                                                                   */
/* ─────────────────────────────────────────────────────────────────────────── */
void tui_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(25);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Terminal does not support colors.\n");
        exit(1);
    }
    start_color();
    use_default_colors();

    if (COLORS >= 256 && can_change_color())
        _colors_256();
    else
        _colors_8();

    bkgd(COLOR_PAIR(CP_NORMAL));
    clear();
    refresh();
}

void tui_cleanup(void) {
    endwin();
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Persistent chrome                                                           */
/* ─────────────────────────────────────────────────────────────────────────── */
void tui_header(const char *username) {
    attron(COLOR_PAIR(CP_STATUS) | A_BOLD);
    mvhline(0, 0, ' ', COLS);
    mvprintw(0, 2, "BANK MANAGEMENT SYSTEM");

    if (username && username[0]) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char right[80];
        snprintf(right, sizeof(right), " %s | %02d:%02d ",
                 username, tm_info->tm_hour, tm_info->tm_min);
        /* strlen is safe here: right is pure ASCII */
        int rlen = (int)strlen(right);
        int rx   = COLS - rlen;
        if (rx < 26) rx = 26; // Keep clear of "BANK MANAGEMENT SYSTEM" (pos 0..23)
        if (rx < COLS) {
            mvprintw(0, rx, "%s", right);
        }
    }
    attroff(COLOR_PAIR(CP_STATUS) | A_BOLD);
    refresh();
}

void tui_footer(const char *hint) {
    attron(COLOR_PAIR(CP_STATUS));
    mvhline(LINES - 1, 0, ' ', COLS);
    if (hint)
        mvprintw(LINES - 1, 2, "%s", hint);
    attroff(COLOR_PAIR(CP_STATUS));
    refresh();
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Window helpers                                                              */
/* ─────────────────────────────────────────────────────────────────────────── */
WINDOW *tui_win_new(int h, int w, int y, int x, const char *title) {
    WINDOW *win = newwin(h, w, y, x);
    wbkgd(win, COLOR_PAIR(CP_NORMAL));

    wattron(win, COLOR_PAIR(CP_BORDER));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(CP_BORDER));

    if (title && title[0]) {
        /* Use tui_dispw so multi-byte titles centre correctly */
        int tvis = tui_dispw(title);
        int tx   = (w - tvis - 2) / 2;
        if (tx < 1) tx = 1;
        wattron(win, COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvwprintw(win, 0, tx, " %s ", title);
        wattroff(win, COLOR_PAIR(CP_TITLE) | A_BOLD);
    }

    wrefresh(win);
    return win;
}

void tui_win_destroy(WINDOW *win) {
    if (!win) return;
    werase(win);
    wrefresh(win);
    delwin(win);
}

void tui_win_status(WINDOW *win, int y, int color_pair, const char *msg) {
    int w = getmaxx(win);
    wattron(win, COLOR_PAIR(color_pair));
    mvwhline(win, y, 1, ' ', w - 2);
    if (msg) {
        int mx = (w - tui_dispw(msg)) / 2;
        if (mx < 2) mx = 2;
        mvwprintw(win, y, mx, "%s", msg);
    }
    wattroff(win, COLOR_PAIR(color_pair));
    wrefresh(win);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Text input                                                                  */
/* ─────────────────────────────────────────────────────────────────────────── */
int tui_input(WINDOW *win, int y, int x, int len,
              char *buf, int maxlen, int password) {
    int pos = 0;
    memset(buf, 0, (size_t)(maxlen + 1));

    /* Draw empty field */
    wattron(win, COLOR_PAIR(CP_FIELD));
    for (int i = 0; i < len; i++) mvwaddch(win, y, x + i, ' ');
    wattroff(win, COLOR_PAIR(CP_FIELD));
    wmove(win, y, x);
    curs_set(1);
    wrefresh(win);

    keypad(win, TRUE);
    int ch;
    while ((ch = wgetch(win)) != '\n' && ch != KEY_ENTER) {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                wattron(win, COLOR_PAIR(CP_FIELD));
                mvwaddch(win, y, x + pos, ' ');
                wattroff(win, COLOR_PAIR(CP_FIELD));
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if (ch == 27) {   /* ESC — abort */
            buf[0] = '\0';
            curs_set(0);
            return 0;
        } else if (ch >= 32 && ch < 127 && pos < maxlen && pos < len) {
            buf[pos] = (char)ch;
            wattron(win, COLOR_PAIR(CP_FIELD));
            mvwaddch(win, y, x + pos, password ? '*' : (chtype)ch);
            wattroff(win, COLOR_PAIR(CP_FIELD));
            pos++;
            wmove(win, y, x + pos);
            wrefresh(win);
        }
    }
    curs_set(0);
    return pos;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* tui_input_int / tui_input_double                                           */
/* ─────────────────────────────────────────────────────────────────────────── */
int tui_input_int(WINDOW *win, int y, int lx, const char *label,
                  int fx, int fw, int *out) {
    wattron(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvwprintw(win, y, lx, "%s", label);
    wattroff(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    wrefresh(win);

    char buf[32];
    if (!tui_input(win, y, fx, fw, buf, 30, 0) || buf[0] == '\0')
        return 0;

    /* Validate: digits only */
    for (int i = 0; buf[i]; i++) {
        if (!isdigit((unsigned char)buf[i])) return 0;
    }
    char *end;
    long v = strtol(buf, &end, 10);
    if (*end != '\0' || v <= 0) return 0;
    *out = (int)v;
    return 1;
}

int tui_input_double(WINDOW *win, int y, int lx, const char *label,
                     int fx, int fw, double *out) {
    wattron(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvwprintw(win, y, lx, "%s", label);
    wattroff(win, COLOR_PAIR(CP_LABEL) | A_BOLD);
    wrefresh(win);

    char buf[48];
    if (!tui_input(win, y, fx, fw, buf, 46, 0) || buf[0] == '\0')
        return 0;

    /* No commas allowed */
    if (strchr(buf, ',')) return 0;

    /* At most 2 decimal places */
    char *dot = strchr(buf, '.');
    if (dot && (int)strlen(dot + 1) > 2) return 0;

    /* Only digits and one dot */
    int dots = 0;
    for (int i = 0; buf[i]; i++) {
        if (buf[i] == '.') { dots++; if (dots > 1) return 0; }
        else if (!isdigit((unsigned char)buf[i])) return 0;
    }

    char *end;
    double v = strtod(buf, &end);
    if (*end != '\0' || v <= 0.0) return 0;
    *out = v;
    return 1;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Item picker  (scrollable popup)                                             */
/* ─────────────────────────────────────────────────────────────────────────── */
int tui_pick_item(const char *title, const char **items, int n,
                  const char *hint) {
    if (n <= 0) return -1;

    /* Determine popup width */
    int mw = tui_dispw(title) + 6;
    for (int i = 0; i < n; i++) {
        int l = tui_dispw(items[i]) + 8;
        if (l > mw) mw = l;
    }
    if (hint) {
        int hl = tui_dispw(hint) + 4;
        if (hl > mw) mw = hl;
    }
    if (mw < 36)       mw = 36;
    if (mw > COLS - 4) mw = COLS - 4;

    /* Visible item rows — leave room for border + title + hint */
    int max_vis = LINES - 8;
    if (max_vis < 1) max_vis = 1;
    int vis = (n < max_vis) ? n : max_vis;

    int mh = vis + 5;   /* border(2) + title gap(1) + hint(1) + spacing(1) */
    int my = (LINES - mh) / 2;
    int mx = (COLS  - mw) / 2;

    WINDOW *pop = newwin(mh, mw, my, mx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));
    keypad(pop, TRUE);

    int sel   = 0;
    int top   = 0;   /* first visible item index */
    int ch;

    while (1) {
        /* Border + title */
        wattron(pop, COLOR_PAIR(CP_BORDER) | A_BOLD);
        box(pop, 0, 0);
        int tx = (mw - tui_dispw(title) - 2) / 2;
        if (tx < 1) tx = 1;
        mvwprintw(pop, 0, tx, " %s ", title);
        wattroff(pop, COLOR_PAIR(CP_BORDER) | A_BOLD);

        /* Scroll: keep sel in view */
        if (sel < top)          top = sel;
        if (sel >= top + vis)   top = sel - vis + 1;

        /* Items */
        for (int i = 0; i < vis; i++) {
            int idx = top + i;
            int row = 2 + i;
            if (idx == sel) {
                wattron(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
                mvwprintw(pop, row, 2, " > %-*s ", mw - 8, items[idx]);
                wattroff(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            } else {
                wattron(pop, COLOR_PAIR(CP_NORMAL));
                mvwprintw(pop, row, 2, "   %-*s ", mw - 8, items[idx]);
                wattroff(pop, COLOR_PAIR(CP_NORMAL));
            }
        }

        /* Scroll indicators */
        if (top > 0) {
            wattron(pop, COLOR_PAIR(CP_DIM));
            mvwprintw(pop, 1, mw - 4, " ^ ");
            wattroff(pop, COLOR_PAIR(CP_DIM));
        }
        if (top + vis < n) {
            wattron(pop, COLOR_PAIR(CP_DIM));
            mvwprintw(pop, mh - 3, mw - 4, " v ");
            wattroff(pop, COLOR_PAIR(CP_DIM));
        }

        /* Hint */
        if (hint) {
            wattron(pop, COLOR_PAIR(CP_DIM));
            int hx = (mw - tui_dispw(hint)) / 2;
            if (hx < 2) hx = 2;
            mvwprintw(pop, mh - 2, hx, "%s", hint);
            wattroff(pop, COLOR_PAIR(CP_DIM));
        }

        wrefresh(pop);

        ch = wgetch(pop);
        if      (ch == KEY_UP   || ch == 'k') sel = (sel > 0)     ? sel - 1 : 0;
        else if (ch == KEY_DOWN || ch == 'j') sel = (sel < n - 1) ? sel + 1 : n - 1;
        else if (ch == '\n' || ch == KEY_ENTER) break;
        else if (ch == 27  || ch == 'q')        { sel = -1; break; }
        else if (ch >= '1' && ch <= '9') {
            int s = ch - '1';
            if (s < n) { sel = s; break; }
        }
    }

    delwin(pop);
    return sel;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Logo                                                                        */
/* ─────────────────────────────────────────────────────────────────────────── */
void tui_draw_logo(WINDOW *win, int starty, int startx) {
    /* Pure ASCII art — guaranteed 1 byte = 1 column, no alignment surprises */
    const char *lines[] = {
        "    _  _____ __  __ ",
        "   / \\|_   _|  \\/  |",
        "  / _ \\ | | | |\\/| |",
        " / ___ \\| | | |  | |",
        "/_/   \\_|_| |_|  |_|",
    };
    int n = (int)(sizeof(lines) / sizeof(lines[0]));

    wattron(win, COLOR_PAIR(CP_TITLE) | A_BOLD);
    for (int i = 0; i < n; i++)
        mvwprintw(win, starty + i, startx, "%s", lines[i]);
    wattroff(win, COLOR_PAIR(CP_TITLE) | A_BOLD);

    wattron(win, COLOR_PAIR(CP_BORDER) | A_BOLD);
    {
        const char *sub = "B A N K   M A N A G E M E N T   S Y S T E M";
        int sub_w = (int)strlen(sub);
        int sub_x = (getmaxx(win) - sub_w) / 2;
        if (sub_x < 0) sub_x = 0;
        mvwprintw(win, starty + n, sub_x, "%s", sub);
    }
    wattroff(win, COLOR_PAIR(CP_BORDER) | A_BOLD);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Modal dialogs                                                               */
/* ─────────────────────────────────────────────────────────────────────────── */

/* Generic info popup.  icon and msg are plain ASCII for reliable centering. */
static void _modal_info(const char *msg, int cpair, const char *icon) {
    /* Cap width so text never bleeds to screen edges */
    int max_mw = COLS - 6;
    if (max_mw < 30) max_mw = 30;

    int inner_w = max_mw - 6;          /* 3-col margin each side inside border */
    int iw      = tui_dispw(icon);

    /* Count how many wrapped lines the message needs */
    /* (reuse tui_print_wrapped logic: crude estimate — 1 pass) */
    int msg_lines = 0;
    {
        const char *p = msg;
        while (*p) {
            int col = 0;
            const unsigned char *s = (const unsigned char *)p;
            const unsigned char *last_sp = NULL; int last_bytes = 0, bytes = 0;
            while (*s) {
                unsigned long cp = 0; int b = 1;
                if      (*s < 0x80)                                { cp = *s;            b = 1; }
                else if ((*s & 0xE0)==0xC0 && *(s+1))             { cp=(*s&0x1F)<<6|(*(s+1)&0x3F);                           b=2; }
                else if ((*s & 0xF0)==0xE0 && *(s+1) && *(s+2))   { cp=(*s&0x0F)<<12|(*(s+1)&0x3F)<<6|(*(s+2)&0x3F);         b=3; }
                else if ((*s & 0xF8)==0xF0 && *(s+1) && *(s+2) && *(s+3)) {
                    cp=(*s&0x07)<<18|(*(s+1)&0x3F)<<12|(*(s+2)&0x3F)<<6|(*(s+3)&0x3F); b=4; }
                int w = (cp >= 0x1F000) ? 2 : 1;
                if (col + w > inner_w) break;
                if (*s == ' ') { last_sp = s; last_bytes = bytes; }
                col += w; bytes += b; s += b;
            }
            if (*s && last_sp) bytes = last_bytes;
            p += bytes; if (*p == ' ') p++;
            msg_lines++;
            if (msg_lines > 8) break;   /* hard cap */
        }
    }
    if (msg_lines < 1) msg_lines = 1;

    /* Height: border(2) + gap(1) + icon(1) + gap(1) + msg lines + gap(1) + prompt(1) */
    int mh = 2 + 1 + 1 + 1 + msg_lines + 1 + 1;
    if (mh < 8) mh = 8;
    int mw = max_mw;
    int my = (LINES - mh) / 2;
    int mx = (COLS  - mw) / 2;

    WINDOW *pop = newwin(mh, mw, my, mx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));

    wattron(pop, COLOR_PAIR(cpair) | A_BOLD);
    box(pop, 0, 0);
    const char *hdr = " STATUS ";
    mvwprintw(pop, 0, (mw - (int)strlen(hdr)) / 2, "%s", hdr);
    wattroff(pop, COLOR_PAIR(cpair) | A_BOLD);

    /* Icon row */
    wattron(pop, COLOR_PAIR(cpair) | A_BOLD);
    mvwprintw(pop, 2, (mw - iw) / 2, "%s", icon);
    wattroff(pop, COLOR_PAIR(cpair) | A_BOLD);

    /* Message — word-wrapped, left-margin 3 inside border */
    tui_print_wrapped(pop, 4, 3, inner_w, msg_lines, CP_NORMAL, msg);

    /* Prompt */
    const char *cont = "[ Press any key ]";
    wattron(pop, COLOR_PAIR(CP_DIM));
    mvwprintw(pop, mh - 2, (mw - (int)strlen(cont)) / 2, "%s", cont);
    wattroff(pop, COLOR_PAIR(CP_DIM));

    wrefresh(pop);
    wgetch(pop);
    delwin(pop);
}

void tui_modal_success(const char *msg) {
    _modal_info(msg, CP_SUCCESS, "[ SUCCESS ]");
}

void tui_modal_error(const char *msg) {
    _modal_info(msg, CP_ERROR, "[  ERROR  ]");
}

int tui_modal_confirm(const char *question) {
    int qw  = tui_dispw(question);
    int mw  = qw + 8;
    if (mw < 48)       mw = 48;
    if (mw > COLS - 4) mw = COLS - 4;
    int mh = 9;
    int my = (LINES - mh) / 2;
    int mx = (COLS  - mw) / 2;

    WINDOW *pop = newwin(mh, mw, my, mx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));
    keypad(pop, TRUE);

    int sel = 1;   /* 1 = YES  0 = NO */
    int ch;

    while (1) {
        wattron(pop, COLOR_PAIR(CP_BORDER) | A_BOLD);
        box(pop, 0, 0);
        const char *hdr = " CONFIRM ";
        mvwprintw(pop, 0, (mw - (int)strlen(hdr)) / 2, "%s", hdr);
        wattroff(pop, COLOR_PAIR(CP_BORDER) | A_BOLD);

        // wattron(pop, COLOR_PAIR(CP_NORMAL));
        // mvwprintw(pop, 2, (mw - qw) / 2, "%s", question);
        // wattroff(pop, COLOR_PAIR(CP_NORMAL));
        /* Wrap the question so it never bleeds past the border */
        tui_print_wrapped(pop, 2, 3, mw - 6, 3, CP_NORMAL, question);

        /* YES button */
        int yes_x = mw / 2 - 14;
        int no_x  = mw / 2 + 2;
        if (sel == 1) {
            wattron(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            mvwprintw(pop, 5, yes_x, "  [ YES ]  ");
            wattroff(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            wattron(pop, COLOR_PAIR(CP_NORMAL));
            mvwprintw(pop, 5, no_x,  "  [ NO  ]  ");
            wattroff(pop, COLOR_PAIR(CP_NORMAL));
        } else {
            wattron(pop, COLOR_PAIR(CP_NORMAL));
            mvwprintw(pop, 5, yes_x, "  [ YES ]  ");
            wattroff(pop, COLOR_PAIR(CP_NORMAL));
            wattron(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            mvwprintw(pop, 5, no_x,  "  [ NO  ]  ");
            wattroff(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        }

        const char *hint = "< > arrows or Y/N,  ENTER confirms";
        wattron(pop, COLOR_PAIR(CP_DIM));
        mvwprintw(pop, 7, (mw - (int)strlen(hint)) / 2, "%s", hint);
        wattroff(pop, COLOR_PAIR(CP_DIM));

        wrefresh(pop);

        ch = wgetch(pop);
        if      (ch == 'y' || ch == 'Y') { sel = 1; }
        else if (ch == 'n' || ch == 'N') { sel = 0; }
        else if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == '\t') { sel = !sel; }
        else if (ch == '\n' || ch == KEY_ENTER)  { break; }
        else if (ch == 27)                        { sel = 0; break; }
    }

    delwin(pop);
    return sel;
}

int tui_modal_menu(const char *title, const char *options[], const char *hint) {
    /* count */
    int n = 0;
    while (options[n]) n++;
    if (n == 0) return -1;

    /* width */
    int mw = tui_dispw(title) + 6;
    for (int i = 0; i < n; i++) {
        int l = tui_dispw(options[i]) + 8;
        if (l > mw) mw = l;
    }
    if (hint) {
        int hl = tui_dispw(hint) + 4;
        if (hl > mw) mw = hl;
    }
    if (mw < 36)       mw = 36;
    if (mw > COLS - 4) mw = COLS - 4;

    int mh = n + 6;
    if (mh > LINES - 2) mh = LINES - 2;
    int my = (LINES - mh) / 2;
    int mx = (COLS  - mw) / 2;

    WINDOW *pop = newwin(mh, mw, my, mx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));
    keypad(pop, TRUE);

    int sel = 0;
    int ch;

    while (1) {
        wattron(pop, COLOR_PAIR(CP_BORDER) | A_BOLD);
        box(pop, 0, 0);
        int tx = (mw - tui_dispw(title) - 2) / 2;
        if (tx < 1) tx = 1;
        mvwprintw(pop, 0, tx, " %s ", title);
        wattroff(pop, COLOR_PAIR(CP_BORDER) | A_BOLD);

        for (int i = 0; i < n; i++) {
            int row = 2 + i;
            if (row >= mh - 2) break;
            if (i == sel) {
                wattron(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
                mvwprintw(pop, row, 2, " > %-*s ", mw - 8, options[i]);
                wattroff(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            } else {
                wattron(pop, COLOR_PAIR(CP_NORMAL));
                mvwprintw(pop, row, 2, "   %-*s ", mw - 8, options[i]);
                wattroff(pop, COLOR_PAIR(CP_NORMAL));
            }
        }

        if (hint) {
            wattron(pop, COLOR_PAIR(CP_DIM));
            int hx = (mw - tui_dispw(hint)) / 2;
            if (hx < 2) hx = 2;
            mvwprintw(pop, mh - 2, hx, "%s", hint);
            wattroff(pop, COLOR_PAIR(CP_DIM));
        }

        wrefresh(pop);

        ch = wgetch(pop);
        if      (ch == KEY_UP   || ch == 'k') sel = (sel > 0)     ? sel - 1 : n - 1;
        else if (ch == KEY_DOWN || ch == 'j') sel = (sel < n - 1) ? sel + 1 : 0;
        else if (ch == '\n' || ch == KEY_ENTER) break;
        else if (ch == 27  || ch == 'q')        { sel = -1; break; }
        else if (ch >= '1' && ch <= '9') {
            int s = ch - '1';
            if (s < n) { sel = s; break; }
        }
    }

    delwin(pop);
    return sel;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Word-wrap printer                                                            */
/* ─────────────────────────────────────────────────────────────────────────── */
int tui_print_wrapped(WINDOW *win, int start_y, int margin_x,
                      int inner_w, int max_lines,
                      int color_pair, const char *text) {
    if (!text || inner_w <= 0 || max_lines <= 0) return 0;

    /* Working copy so we can mutate it */
    char buf[1024];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    int line = 0;
    char *p = buf;

    while (*p && line < max_lines) {
        /* How many bytes fit on one visual line? */
        int col = 0, bytes = 0;
        const unsigned char *s = (const unsigned char *)p;
        const unsigned char *last_space_pos = NULL;
        int   last_space_bytes = 0;

        while (*s) {
            /* Decode one UTF-8 codepoint */
            unsigned long cp = 0; int b = 1;
            if      (*s < 0x80)                                    { cp = *s;            b = 1; }
            else if ((*s & 0xE0) == 0xC0 && *(s+1))               { cp = (*s&0x1F)<<6  | (*(s+1)&0x3F);                           b = 2; }
            else if ((*s & 0xF0) == 0xE0 && *(s+1) && *(s+2))     { cp = (*s&0x0F)<<12 | (*(s+1)&0x3F)<<6 | (*(s+2)&0x3F);       b = 3; }
            else if ((*s & 0xF8) == 0xF0 && *(s+1) && *(s+2) && *(s+3)) {
                                                                      cp = (*s&0x07)<<18 | (*(s+1)&0x3F)<<12 | (*(s+2)&0x3F)<<6 | (*(s+3)&0x3F); b = 4; }
            int w = (cp >= 0x1F000) ? 2 : 1;

            if (col + w > inner_w) break;   /* would overflow — stop */
            if (*s == ' ') { last_space_pos = s; last_space_bytes = bytes; }
            col  += w;
            bytes += b;
            s    += b;
        }

        /* If we stopped mid-word and there was a previous space, break there */
        if (*s && last_space_pos) {
            bytes = last_space_bytes;
        }

        /* Print this line */
        char save = p[bytes];
        p[bytes] = '\0';
        wattron(win, COLOR_PAIR(color_pair));
        mvwprintw(win, start_y + line, margin_x, "%s", p);
        wattroff(win, COLOR_PAIR(color_pair));
        p[bytes] = save;

        /* Advance past the printed bytes (skip leading space on next line) */
        p += bytes;
        if (*p == ' ') p++;
        line++;
    }
    return line;
}