/*
 * MacSSL - coreHTTP Configuration for Classic Mac OS
 * Configuration for coreHTTP library tailored for Classic Mac OS with Retro68
 */

#ifndef CORE_HTTP_CONFIG_H_
#define CORE_HTTP_CONFIG_H_

/* Include necessary headers */
#include <stdio.h>

/**
 * @brief The HTTP header "User-Agent" value for our Classic Mac client.
 */
#define HTTP_USER_AGENT_VALUE    "MacSSL/1.0"

/**
 * @brief Network retry timeout in milliseconds.
 * Use a longer timeout for Classic Mac OS slower networking.
 */
#define HTTP_RECV_RETRY_TIMEOUT_MS    ( 100U )

/**
 * @brief Send retry timeout in milliseconds.
 * Use a longer timeout for Classic Mac OS slower networking.
 */
#define HTTP_SEND_RETRY_TIMEOUT_MS    ( 100U )

/**
 * @brief Map coreHTTP logging to our existing logging system.
 * For now, disable logging to avoid compilation issues.
 * We can enable it later once the basic integration works.
 */
#define LogError( message )   /* Disabled for now */
#define LogWarn( message )    /* Disabled for now */
#define LogInfo( message )    /* Disabled for now */
#define LogDebug( message )   /* Disabled for now */

#endif /* ifndef CORE_HTTP_CONFIG_H_ */