#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <menu.h>
#include <form.h>

/* ── Color pair IDs ──────────────────────────────────────────────────────── */
#define CP_NORMAL    1   /* Gold on black       – body text                 */
#define CP_BORDER    2   /* Green on black      – borders / dividers        */
#define CP_SELECTED  3   /* Black on gold       – highlighted item          */
#define CP_TITLE     4   /* Bright gold, bold   – window / section titles   */
#define CP_LABEL     5   /* Green on black      – form field labels         */
#define CP_FIELD     6   /* White on near-black – input field background    */
#define CP_ERROR     7   /* Red on black        – error messages            */
#define CP_SUCCESS   8   /* Bright green        – success messages          */
#define CP_STATUS    9   /* Black on dark-green – header / footer bars      */
#define CP_DIM       10  /* Dim gold            – secondary / hint text     */

/* Custom 256-color terminal slots (only used when COLORS >= 256) */
#define CLR_GOLD     16
#define CLR_GOLD_DIM 17
#define CLR_DKGREEN  18
#define CLR_DKBG     19

/* ── Lifecycle ───────────────────────────────────────────────────────────── */
void tui_init(void);
void tui_cleanup(void);

/* ── Persistent chrome ───────────────────────────────────────────────────── */
/* Draw the full-width header bar (row 0).  username may be NULL. */
void tui_header(const char *username);
/* Draw the full-width footer hint bar (last row). */
void tui_footer(const char *hint);

/* ── Utility ─────────────────────────────────────────────────────────────── */
/* Visible terminal column-width of a UTF-8 string.
   ASCII / 2-byte / 3-byte BMP chars = 1 col each.
   4-byte emoji codepoints = 2 cols each. */
int tui_dispw(const char *s);

/* ── Window helpers ──────────────────────────────────────────────────────── */
WINDOW *tui_win_new(int h, int w, int y, int x, const char *title);
void    tui_win_destroy(WINDOW *win);

/* Print a full-width status line at row y inside win.
   color_pair should be CP_ERROR, CP_SUCCESS, CP_DIM, etc. */
void tui_win_status(WINDOW *win, int y, int color_pair, const char *msg);

/* ── Text input ──────────────────────────────────────────────────────────── */
/* Read at most maxlen chars into buf at window position (y, x).
   Draws a white-on-dark field strip of width len columns.
   password != 0 masks each char as '*'.
   Returns chars read; 0 means ESC / empty. */
int tui_input(WINDOW *win, int y, int x, int len,
              char *buf, int maxlen, int password);

/* Read a validated integer.
   Draws label at (y, lx) in CP_LABEL and an input field at (y, fx) width fw.
   Returns 1 on success (sets *out), 0 on ESC / empty / invalid. */
int tui_input_int(WINDOW *win, int y, int lx, const char *label,
                  int fx, int fw, int *out);

/* Read a validated positive decimal with at most 2 decimal places.
   Returns 1 on success (sets *out), 0 on ESC / empty / invalid. */
int tui_input_double(WINDOW *win, int y, int lx, const char *label,
                     int fx, int fw, double *out);

/* ── Item picker ─────────────────────────────────────────────────────────── */
/* Show a scrollable centred popup listing n items.
   Returns the 0-based index chosen, or -1 on ESC/q. */
int tui_pick_item(const char *title, const char **items, int n,
                  const char *hint);

/* ── Logo ────────────────────────────────────────────────────────────────── */
void tui_draw_logo(WINDOW *win, int starty, int startx);

/* ── Modal dialogs ───────────────────────────────────────────────────────── */
void tui_modal_success(const char *msg);
void tui_modal_error  (const char *msg);

/* Returns 1 for YES, 0 for NO. */
int  tui_modal_confirm(const char *question);

/* options[] is a NULL-terminated array of strings.
   Returns 0-based index of choice, or -1 on ESC. */
int  tui_modal_menu(const char *title,
                    const char *options[],
                    const char *hint);

#endif /* TUI_H */