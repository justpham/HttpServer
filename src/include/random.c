
#include "random.h"

void
random_string(char *dest, size_t length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "0123456789";
    size_t charset_size = sizeof(charset) - 1;

    for (size_t i = 0; i < length; i++) {
        int key = rand() % charset_size;
        dest[i] = charset[key];
    }
    dest[length] = '\0'; // null terminator
}
