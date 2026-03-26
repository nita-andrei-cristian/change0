#include <stddef.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CLAMP(mi,mx,v) ((v) < (mi) ? (mi) : ((v) > (mx) ? (mx) : (v)))

// ai generated strnlen
size_t mystrnlen(const char* s, size_t maxlen);
