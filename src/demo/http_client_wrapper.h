/*
 * MacSSL - coreHTTP Client Header for Classic Mac OS
 *
 * This header declares the HTTP client functions that use the coreHTTP
 * library for robust and standards-compliant HTTP communication.
 */

#ifndef HTTP_CLIENT_WRAPPER_H_
#define HTTP_CLIENT_WRAPPER_H_

#include <Types.h>

/**
 * @brief ConnectToServer implementation using coreHTTP.
 *
 * This function connects to a server and fetches data using the coreHTTP
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

#endif /* HTTP_CLIENT_WRAPPER_H_ */