#include "string.h"

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

size_t strnlen(const char *s, size_t max_len) {
    size_t n = 0;
    while (n < max_len && s[n]) n++;
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) { a++; b++; n--; }
    return n ? (int)(uint8_t)*a - (int)(uint8_t)*b : 0;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++))
        ;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

char *strcat(char *dst, const char *src) {
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++))
        ;
    return dst;
}

void *memset(void *dst, int val, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    while (n--) *d++ = (uint8_t)val;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) return dst;

    /* Align both pointers for wider copies first if possible. */
    while (n && ((((uint32_t)(size_t)d) & 3u) || (((uint32_t)(size_t)s) & 3u))) {
        *d++ = *s++;
        n--;
    }

    uint32_t *dw = (uint32_t *)d;
    const uint32_t *sw = (const uint32_t *)s;

    while (n >= 4) {
        *dw++ = *sw++;
        n -= 4;
    }

    d = (uint8_t *)dw;
    s = (const uint8_t *)sw;

    while (n--) {
        *d++ = *s++;
    }

    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) return dst;

    if (d < s || d >= s + n) {
        return memcpy(dst, src, n);
    }

    d += n;
    s += n;
    while (n--) {
        *--d = *--s;
    }

    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;

    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
    }

    return 0;
}

char *strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) return NULL;
    }
    return (char *)s;
}

char *strstr(const char *haystack, const char *needle) {
    size_t needle_len;

    if (!haystack || !needle) return NULL;

    needle_len = strlen(needle);
    if (needle_len == 0) return (char *)haystack;

    while (*haystack) {
        if (strncmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }

    return NULL;
}

static bool is_delim_char(char c, const char *delim) {
    while (*delim) {
        if (c == *delim++) return true;
    }
    return false;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token_start;
    char *p;

    if (!saveptr) return NULL;
    if (!delim || !*delim) delim = " ";

    p = str ? str : *saveptr;
    if (!p) return NULL;

    while (*p && is_delim_char(*p, delim)) {
        p++;
    }

    if (*p == '\0') {
        *saveptr = p;
        return NULL;
    }

    token_start = p;

    while (*p && !is_delim_char(*p, delim)) {
        p++;
    }

    if (*p) {
        *p = '\0';
        p++;
    }

    *saveptr = p;
    return token_start;
}

char *strtok(char *str, const char *delim) {
    static char *state;
    return strtok_r(str, delim, &state);
}

void itoa(int value, char *buf, int base) {
    char tmp[34];
    int i = 0;
    bool neg = false;

    if (value < 0 && base == 10) {
        neg = true;
        value = -value;
    }

    uint32_t uval = (uint32_t)value;
    do {
        int d = uval % base;
        tmp[i++] = d < 10 ? '0' + d : 'A' + d - 10;
        uval /= base;
    } while (uval);

    if (neg) tmp[i++] = '-';

    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

void utoa(uint32_t value, char *buf, int base) {
    char tmp[34];
    int i = 0;

    do {
        int d = value % base;
        tmp[i++] = d < 10 ? '0' + d : 'A' + d - 10;
        value /= base;
    } while (value);

    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
