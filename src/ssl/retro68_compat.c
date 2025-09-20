/*
 * retro68_compat.c
 *
 * Implementation of compatibility functions for MbedTLS
 * with Retro68 GCC toolchain on Classic Mac OS
 */

#include "retro68_compat.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Memory.h>

/*
 * Memory allocation functions for Mac OS Memory Manager
 * Note: mbedtls_calloc/mbedtls_free are now provided by mbedtls platform.c
 * These are our internal Mac OS memory functions
 */
void* mac_calloc(size_t count, size_t size) {
    unsigned long totalSize;
    totalSize = (unsigned long)(count * size);
    return NewPtrClear(totalSize);
}

void mac_free(void* ptr) {
    if (ptr != NULL)
        DisposePtr((Ptr)ptr);
}

/*
 * Simple entropy function for MbedTLS
 * Uses Mac OS TickCount and other sources for basic entropy
 */
int mac_entropy_func(void *data, unsigned char *output, size_t len) {
    size_t i;
    unsigned long ticks = TickCount();

    (void)data; /* Unused parameter */

    /* Simple entropy based on system ticks and memory addresses */
    for (i = 0; i < len; i++) {
        output[i] = (unsigned char)((ticks + i + (unsigned long)output) & 0xFF);
        ticks = (ticks << 1) ^ ticks;  /* Simple mixing */
    }

    return 0;
}

