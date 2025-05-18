// string_utils.c
#include "string_utils.h"
#include <ctype.h>

// Helper function to convert a string to lowercase
void toLower(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}