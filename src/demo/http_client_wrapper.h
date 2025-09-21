/*
 * MacSSL - coreHTTP Client Wrapper Header for Classic Mac OS
 *
 * This header declares the wrapper functions for the new coreHTTP-based
 * HTTP client implementation.
 */

#ifndef HTTP_CLIENT_WRAPPER_H_
#define HTTP_CLIENT_WRAPPER_H_

#include <Types.h>

/**
 * @brief New implementation of ConnectToServer using coreHTTP.
 *
 * This function provides the same interface as the original ConnectToServer()
 * but uses coreHTTP internally for better HTTP handling.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus ConnectToServer_New(void);

#endif /* HTTP_CLIENT_WRAPPER_H_ */