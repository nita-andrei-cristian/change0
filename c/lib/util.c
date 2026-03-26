#include "util.h"

size_t mystrnlen(const char* s, size_t maxlen) {
    size_t i;
    for (i = 0; i < maxlen && s[i] != '\0'; i++);
    return i;
}

