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
 * Implementation of snprintf for Classic Mac OS
 * GCC supports the functions, but the Mac OS runtime might not
 */
int mac_snprintf(char* str, size_t size, const char* format, ...) {
    int result;
    va_list args;
    char temp_buffer[4096]; /* Adjust size as needed */

    /* If buffer size is 0 or negative, do nothing */
    if (size <= 0) return -1;

    /* Format the string using vsprintf */
    va_start(args, format);

    /* Use a temporary buffer to avoid overflow */
    result = vsprintf(temp_buffer, format, args);

    va_end(args);

    /* Copy result to output buffer with size limit */
    if (result >= 0) {
        size_t copy_size = (result < size - 1) ? result : size - 1;
        memcpy(str, temp_buffer, copy_size);
        str[copy_size] = '\0'; /* Ensure null termination */
    } else {
        if (size > 0) str[0] = '\0';
    }

    return result;
}

/*
 * Implementation of vsnprintf for Classic Mac OS
 */
int mac_vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    char temp_buffer[4096]; /* Adjust size as needed */
    int result;

    /* If buffer size is 0 or negative, do nothing */
    if (size <= 0) return -1;

    /* Use a temporary buffer to avoid overflow */
    result = vsprintf(temp_buffer, format, ap);

    /* Copy result to output buffer with size limit */
    if (result >= 0) {
        size_t copy_size = (result < size - 1) ? result : size - 1;
        memcpy(str, temp_buffer, copy_size);
        str[copy_size] = '\0'; /* Ensure null termination */
    } else {
        if (size > 0) str[0] = '\0';
    }

    return result;
}


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

