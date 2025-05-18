#include "terminal_utils.h"
#include <stdio.h>
#include <unistd.h> 
#include <termios.h>

int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);           // Save current terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);         // Disable buffered I/O and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // Apply new settings
    ch = getchar();                           // Read one char (no Enter needed)
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore old settings
    return ch;
}