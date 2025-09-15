/*
 * mbedtls_compat.h
 *
 * Compatibility header for using MbedTLS with Retro68 GCC toolchain
 * on Classic Mac OS
 */
#ifndef MBEDTLS_COMPAT_H
#define MBEDTLS_COMPAT_H

#ifdef RETRO68_BUILD
/* For Retro68 build, include Mac headers first to avoid conflicts */
#include <Types.h>
#include <Memory.h>
#include <OSUtils.h>

/* Then include standard headers */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#else
/* Include our stdint replacements for CodeWarrior */
#include "mac_stdint.h"

/* Include Mac OS headers we'll need */
#include <Types.h>
#include <Memory.h>
#include <OSUtils.h>
#endif

#ifndef RETRO68_BUILD
/* Define size_t if not already defined (CodeWarrior only) */
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

/* Define va_list if not already defined (CodeWarrior only) */
#ifndef _VA_LIST
#define _VA_LIST
typedef char* va_list;
#endif

/*
 * Handle type conflicts with Mac OS types
 * Temporarily redefine Mac OS types before including mbedtls headers
 */
#define Byte        Mac_Byte
#define Word        Mac_Word
#define Boolean     Mac_Boolean
#define Size        Mac_Size

/* Define C99 types and functions needed by mbedtls (CodeWarrior only) */
#ifndef bool
typedef int                 bool;
#define true                1
#define false               0
#endif

/* Define inline support (CodeWarrior only) */
#ifndef inline
#define inline
#endif

/* Time functions - Classic Mac OS uses unsigned long for time */
#ifndef time_t
typedef unsigned long       time_t;
#endif

#endif /* !RETRO68_BUILD */

/*
 * Define platform endianness
 * Classic Mac on 68K is big-endian
 */
#define MBEDTLS_BYTE_ORDER MBEDTLS_BIG_ENDIAN

/*
 * Define snprintf replacement
 * Classic Mac OS doesn't have standard snprintf
 */
int mac_snprintf(char* str, size_t size, const char* format, ...);
int mac_vsnprintf(char* str, size_t size, const char* format, va_list ap);

#ifndef RETRO68_BUILD
#define snprintf mac_snprintf
#define vsnprintf mac_vsnprintf
#endif

/*
 * Memory allocation functions
 * Map to Mac OS Memory Manager
 */
void* mbedtls_calloc(size_t count, size_t size);
void mbedtls_free(void* ptr);

/* Simple printf implementation */
int mac_printf(const char *format, ...);

/* Include this before any mbedtls headers */
#endif /* MBEDTLS_COMPAT_H */