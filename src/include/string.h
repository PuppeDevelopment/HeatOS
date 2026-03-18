#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t max_len);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
void  *memset(void *dst, int val, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
char  *strchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);
char  *strtok_r(char *str, const char *delim, char **saveptr);
char  *strtok(char *str, const char *delim);
void   itoa(int value, char *buf, int base);
void   utoa(uint32_t value, char *buf, int base);

#endif
