/*
 * MacSSL - coreHTTP Transport Interface Header for Classic Mac OS
 *
 * This header declares the transport interface functions for integrating
 * coreHTTP with our existing SSL and OpenTransport networking.
 */

#ifndef HTTP_TRANSPORT_MAC_H_
#define HTTP_TRANSPORT_MAC_H_

#include <Types.h>
#include <OpenTptInternet.h>

/* coreHTTP transport interface */
#include "transport_interface.h"

/* Our SSL wrapper */
#include "ssl_wrapper.h"

/* Network context structure for Classic Mac OS */
struct NetworkContext
{
    SSLState* pSSLState;        /* Pointer to our SSL state */
    InetAddress serverAddr;     /* Server address */
    char hostname[256];         /* Server hostname for SNI */
    Boolean isConnected;        /* Connection status */
};

typedef struct NetworkContext NetworkContext_t;

/**
 * @brief Initialize a network context for HTTP transport.
 *
 * @param[out] pNetworkContext Network context to initialize.
 * @param[in] pSSLState SSL state to use for the connection.
 * @param[in] hostname Server hostname.
 *
 * @return noErr on success, error code on failure.
 */
OSStatus MacSSL_InitializeNetworkContext( NetworkContext_t * pNetworkContext,
                                          SSLState * pSSLState,
                                          const char * hostname );

/**
 * @brief Connect the network context using our existing SSL connection.
 *
 * @param[in,out] pNetworkContext Network context to connect.
 * @param[in] pServerAddr Server address to connect to.
 *
 * @return noErr on success, error code on failure.
 */
OSStatus MacSSL_ConnectNetworkContext( NetworkContext_t * pNetworkContext,
                                       InetAddress * pServerAddr );

/**
 * @brief Disconnect and cleanup the network context.
 *
 * @param[in,out] pNetworkContext Network context to disconnect.
 */
void MacSSL_DisconnectNetworkContext( NetworkContext_t * pNetworkContext );

/**
 * @brief Setup a TransportInterface_t with our Mac-specific implementations.
 *
 * @param[out] pTransportInterface Transport interface to setup.
 * @param[in] pNetworkContext Network context to use.
 */
void MacSSL_SetupTransportInterface( TransportInterface_t * pTransportInterface,
                                     NetworkContext_t * pNetworkContext );

/**
 * @brief Transport receive implementation.
 * (Used internally by coreHTTP)
 */
int32_t MacSSL_TransportRecv( NetworkContext_t * pNetworkContext,
                              void * pBuffer,
                              size_t bytesToRecv );

/**
 * @brief Transport send implementation.
 * (Used internally by coreHTTP)
 */
int32_t MacSSL_TransportSend( NetworkContext_t * pNetworkContext,
                              const void * pBuffer,
                              size_t bytesToSend );

#endif /* HTTP_TRANSPORT_MAC_H_ */