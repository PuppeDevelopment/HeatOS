#include "string.h"

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
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

    /* Align both pointers for wider copies first. */
    while (n && ((((uint32_t)(size_t)d) & 3u) || (((uint32_t)(size_t)s) & 3u))) {
        *d++ = *s++;
        n--;
    }

    uint32_t *dw = (uint32_t *)d;
    const uint32_t *sw = (const uint32_t *)s;

    while (n >= 16) {
        dw[0] = sw[0];
        dw[1] = sw[1];
        dw[2] = sw[2];
        dw[3] = sw[3];
        dw += 4;
        sw += 4;
        n -= 16;
    }

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
