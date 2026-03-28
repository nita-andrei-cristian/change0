#ifndef CHANGE_UTIL_FUNCTIONS
#define CHANGE_UTIL_FUNCTIONS
#include <stddef.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CLAMP(mi,mx,v) ((v) < (mi) ? (mi) : ((v) > (mx) ? (mx) : (v)))

// ai generated strnlen
size_t mystrnlen(const char* s, size_t maxlen);
#endif

#ifndef READ_FILE_FUNC
char* readFile(char* filename);
#endif

#ifndef CONVERSION_FUNCTIONS
#define CONVERSION_FUNCTIONS
void itoa(int n, char s[]);
void ltoa(long n, char s[]);
void dtoa(double n, char s[], int precision);
double atod(char s[], int precision);
#endif

#ifndef STRING_SEARCH_FUNCTIONS
#define STRING_SEARCH_FUNCTIONS

char* searchFirstDigit(char *source);

char* searchFirstDigitWithComma(char *source);

char* searchFirstNonDigitWithComma(char *source);

char* searchFirstNonDigit(char *source);

#endif
