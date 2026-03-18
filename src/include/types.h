#ifndef TYPES_H
#define TYPES_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

#if defined(__x86_64__) || defined(_M_X64)
typedef unsigned long long size_t;
typedef unsigned long long uintptr_t;
typedef signed long long   intptr_t;
#else
typedef unsigned int       size_t;
typedef unsigned int       uintptr_t;
typedef signed int         intptr_t;
#endif

#ifndef __cplusplus
typedef int                bool;

#define true  1
#define false 0
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  nullptr
#else
#define NULL  ((void*)0)
#endif
#endif

#endif
