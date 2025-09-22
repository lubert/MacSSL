/*
 * MacPlatform.c
 *
 * Implementation of platform compatibility functions for MbedTLS
 * with Retro68 GCC toolchain on Classic Mac OS
 */

#include "MacPlatform.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Memory.h>

void* mac_calloc(size_t count, size_t size) {
    unsigned long totalSize;
    totalSize = (unsigned long)(count * size);
    return NewPtrClear(totalSize);
}

void mac_free(void* ptr) {
    if (ptr != NULL)
        DisposePtr((Ptr)ptr);
}


