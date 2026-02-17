#ifndef TERMINAL_UTILS_H
#define TERMINAL_UTILS_H

/* getch() is provided by ncurses as a macro when ncurses.h is included.
   Only declare our own implementation when ncurses is NOT in use. */
#ifndef getch
int getch(void);
#endif

#endif /* TERMINAL_UTILS_H */