/*
 * MacPlatform.h
 *
 * Platform compatibility header for using MbedTLS with Retro68 GCC toolchain
 * on Classic Mac OS
 *
 * This replaces the CodeWarrior-specific compatibility layer
 */

#ifndef MAC_PLATFORM_H
#define MAC_PLATFORM_H

/* Include Mac OS headers first to avoid conflicts */
#include <Types.h>
#include <Memory.h>
#include <OSUtils.h>
#include <Sound.h>

/* GCC supports standard C99 stdint.h, unlike CodeWarrior */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Handle type conflicts with Mac OS types
 * Since we included Mac headers first, Boolean is already defined correctly
 * For Retro68, we don't need to redefine these - just avoid conflicts
 */

/*
 * Define platform endianness
 * Classic Mac on 68K is big-endian
 */
#define MBEDTLS_BYTE_ORDER MBEDTLS_BIG_ENDIAN

/*
 * Time functions - Classic Mac OS uses unsigned long for time
 * and doesn't have time_t/struct tm as in modern C
 * Note: Retro68 already defines time_t, so we don't redefine it
 */

/*
 * Internal Mac OS memory allocation functions
 * (mbedtls_calloc/mbedtls_free are now provided by mbedtls platform.c)
 */
void* mac_calloc(size_t count, size_t size);
void mac_free(void* ptr);

/* Note: SysBeep is now provided by Sound.h */
/* Note: printf is provided by stdio.h - no need for mac_printf */

/*
 * Since we're using GCC now, we can use real 64-bit integers
 * No need for the struct-based 64-bit emulation from CodeWarrior
 */
#define MBEDTLS_HAVE_INT64

/*
 * GCC-specific optimizations
 */
#ifdef __GNUC__
#define MBEDTLS_INLINE static inline __attribute__((always_inline))
#else
#define MBEDTLS_INLINE static inline
#endif

/* Include this before any mbedtls headers */
#endif /* MAC_PLATFORM_H */