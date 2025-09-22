/*
 * PostMac Demo HTTP Functions
 *
 * These are demo-specific HTTP functions that depend on UI globals
 * and should not be part of the core MacSSL library.
 */

#ifndef DEMO_HTTP_H
#define DEMO_HTTP_H

#include <Types.h>

/**
 * @brief Connect to server and perform HTTPS request using coreHTTP.
 *
 * This is the main function for making HTTPS requests. It replaces the older
 * SSL-only implementation with a complete HTTP client using the coreHTTP
 * library for robust HTTP handling.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus ConnectToServer(void);

/**
 * @brief TestSSLHandshake implementation using coreHTTP transport layer.
 *
 * This function tests just the SSL connection without sending HTTP data,
 * using the coreHTTP transport layer for connection management.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus TestSSLHandshake(void);

#endif /* DEMO_HTTP_H */