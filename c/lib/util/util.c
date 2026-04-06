#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>

size_t mystrnlen(const char* s, size_t maxlen) {
    size_t i;
    for (i = 0; i < maxlen && s[i] != '\0'; i++);
    return i;
}


char* searchFirstDigit(char *source){
	char* dest = source;
	do{
		if (*dest >= '0' && *dest <= '9') return dest;
	}while(*(++dest) != '\0');
	return NULL;
}

char* searchFirstDigitWithComma(char *source){
	char* dest = source;
	do{
		if (*dest >= '0' && *dest <= '9' || *dest == '.' || *dest == ',') return dest;
	}while(*(++dest) != '\0');
	return NULL;
}

char* searchFirstNonDigitWithComma(char *source){
	char* dest = source;
	do{
		if ((*dest < '0' || *dest > '9') && *dest != '.' && *dest != ',') return dest;
	}while(*(++dest) != '\0');
	return NULL;
}

char* searchFirstNonDigit(char *source){
	char* dest = source;
	do{
		if (*dest < '0' || *dest > '9') return dest;
	}while(*(++dest) != '\0');
	return NULL;
}

char* readFile(char* filename, size_t *size)
{
	FILE* f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "File %s not found.\n", filename);
		return NULL;
	};

	fseek(f, 0, SEEK_END);
	*size = (size_t) ftell(f);
	rewind(f);

	char* buffer = malloc(*size + 1);
	if (!buffer){
		fclose(f);
		fprintf(stderr, "Error allocating \"%s\" in a buffer\n", filename);
		return NULL;
	}

	fread(buffer, 1, *size, f);
	buffer[*size] = '\0';

	fclose(f);
	return buffer;	
}
/*

itoa appeared in the first edition of
Kernighan and Ritchie's
The C Programming Language, 
on page 60.

*/

// Source - https://stackoverflow.com/a/29544416
// Posted by haccks, modified by community. See post 'Timeline' for change history
// Retrieved 2026-03-09, License - CC BY-SA 3.0

 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }

}  

// Source - https://stackoverflow.com/a/29544416
// Posted by haccks, modified by community. See post 'Timeline' for change history
// Retrieved 2026-03-09, License - CC BY-SA 3.0

 /* itoa:  convert n to characters in s */
void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}  

/*
int atoi(char s[]){
	int n = 0;
	char* dest = s;
	while (*dest != '\0' && *dest >= '0' && *dest <= '9'){
		n *= 10 + *dest - '0';
	}

	return n;
}*/

 void ltoa(long n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}  

// This was findable on stack overlflow but I lost it so i head to generate with AI.

void dtoa(double n, char s[], int precision)
{
    int i = 0, sign;

    if ((sign = n < 0)) {
        n = -n;
    }

    int intPart = (int)n;
    double fracPart = n - intPart;

    /* convert integer part */
    do {
        s[i++] = intPart % 10 + '0';
    } while ((intPart /= 10) > 0);

    if (sign)
        s[i++] = '-';

    s[i] = '\0';     
    reverse(s);       

    i = strlen(s);  

    /* add decimal point */
    s[i++] = '.';

    /* convert fractional part */
    for (int j = 0; j < precision; j++) {
        fracPart *= 10;
        int digit = (int)fracPart;
        s[i++] = digit + '0';
        fracPart -= digit;
    }

    s[i] = '\0';
}

double atod(char s[], int precision)
{
	double result = 0.0;
	char* tip = s;
	double i = 1;

	while (*tip != '\0' && tip < s + precision){
		if (*tip == '.' || *tip == ',') {
			tip++;
			continue;
		}
		result = result * 10 + (double)(*tip - '0');
		tip++;
		i*=10;
	}

	if (i == 0) return 0.0;

	return (result / i) * 10;
}

