/*
 * MacSSL - coreHTTP Transport Interface for Classic Mac OS
 *
 * This file implements the coreHTTP transport interface using our existing
 * SSL_* functions and OpenTransport networking layer.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Our existing SSL and networking headers first */
#include "../ssl/SSLWrapper.h"

/* coreHTTP transport interface */
#include "../coreHTTP/source/interface/transport_interface.h"

/* Our transport interface header (includes NetworkContext definition) */
#include "MacHTTPTransport.h"

/* AppendLogText removed - library should not depend on demo UI functions */

/* OpenTransport headers */
#include <OpenTransport.h>
#include <OpenTptInternet.h>

/* Network context structure is now defined in MacHTTPTransport.h */

/**
 * @brief Transport receive implementation using our SSL_Receive function.
 *
 * @param[in] pNetworkContext Network context containing SSL state.
 * @param[in] pBuffer Buffer to receive data into.
 * @param[in] bytesToRecv Number of bytes to receive.
 *
 * @return Number of bytes received, 0 for retry, or negative for error.
 */
int32_t MacSSL_TransportRecv( NetworkContext_t * pNetworkContext,
                              void * pBuffer,
                              size_t bytesToRecv )
{
    OSStatus err;
    size_t bytesReceived = 0;
    int32_t returnValue = 0;

    /* Validate parameters */
    if( (pNetworkContext == NULL) || (pBuffer == NULL) || (bytesToRecv == 0) )
    {
        if (pNetworkContext && pNetworkContext->logFunc) {
            pNetworkContext->logFunc("MacSSL_TransportRecv: Invalid parameters");
        }
        return -1;
    }

    if( (pNetworkContext->pSSLState == NULL) || (!pNetworkContext->isConnected) )
    {
        if (pNetworkContext->logFunc) {
            pNetworkContext->logFunc("MacSSL_TransportRecv: Not connected");
        }
        return -1;
    }

    /* Call our existing SSL_Receive function */
    err = SSL_Receive( pNetworkContext->pSSLState,
                       pBuffer,
                       bytesToRecv,
                       &bytesReceived,
                       pNetworkContext->logFunc );

    if( err == noErr )
    {
        /* Successful read */
        returnValue = (int32_t)bytesReceived;
    }
    else if( bytesReceived == 0 )
    {
        /* No data available - return 0 for retry */
        returnValue = 0;
    }
    else
    {
        /* Error occurred */
        if (pNetworkContext->logFunc) {
            char errorMsg[100];
            sprintf(errorMsg, "MacSSL_TransportRecv: SSL_Receive error %d", (int)err);
            pNetworkContext->logFunc(errorMsg);
        }
        returnValue = -1;
    }

    return returnValue;
}

/**
 * @brief Transport send implementation using our SSL_Send function.
 *
 * @param[in] pNetworkContext Network context containing SSL state.
 * @param[in] pBuffer Buffer containing data to send.
 * @param[in] bytesToSend Number of bytes to send.
 *
 * @return Number of bytes sent, 0 for retry, or negative for error.
 */
int32_t MacSSL_TransportSend( NetworkContext_t * pNetworkContext,
                              const void * pBuffer,
                              size_t bytesToSend )
{
    OSStatus err;
    size_t bytesSent = 0;
    int32_t returnValue = 0;

    /* Validate parameters */
    if( (pNetworkContext == NULL) || (pBuffer == NULL) || (bytesToSend == 0) )
    {
        if (pNetworkContext && pNetworkContext->logFunc) {
            pNetworkContext->logFunc("MacSSL_TransportSend: Invalid parameters");
        }
        return -1;
    }

    if( (pNetworkContext->pSSLState == NULL) || (!pNetworkContext->isConnected) )
    {
        if (pNetworkContext->logFunc) {
            pNetworkContext->logFunc("MacSSL_TransportSend: Not connected");
        }
        return -1;
    }

    /* Call our existing SSL_Send function */
    err = SSL_Send( pNetworkContext->pSSLState,
                    pBuffer,
                    bytesToSend,
                    &bytesSent,
                    pNetworkContext->logFunc );

    if( err == noErr )
    {
        /* Successful send */
        returnValue = (int32_t)bytesSent;
    }
    else if( bytesSent == 0 )
    {
        /* Buffer full - return 0 for retry */
        returnValue = 0;
    }
    else
    {
        /* Error occurred */
        if (pNetworkContext->logFunc) {
            char errorMsg[100];
            sprintf(errorMsg, "MacSSL_TransportSend: SSL_Send error %d", (int)err);
            pNetworkContext->logFunc(errorMsg);
        }
        returnValue = -1;
    }

    return returnValue;
}

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
                                          const char * hostname,
                                          TransportLogCallback logFunc )
{
    if( (pNetworkContext == NULL) || (pSSLState == NULL) || (hostname == NULL) )
    {
        return paramErr;
    }

    /* Clear the context */
    memset( pNetworkContext, 0, sizeof(NetworkContext_t) );

    /* Set up the context */
    pNetworkContext->pSSLState = pSSLState;
    pNetworkContext->isConnected = false;
    pNetworkContext->logFunc = logFunc;

    /* Copy hostname (with length check) */
    if( strlen(hostname) >= sizeof(pNetworkContext->hostname) )
    {
        if (logFunc) {
            logFunc("MacSSL_InitializeNetworkContext: Hostname too long");
        }
        return paramErr;
    }
    strcpy( pNetworkContext->hostname, hostname );

    return noErr;
}

/**
 * @brief Connect the network context using our existing SSL connection.
 *
 * @param[in,out] pNetworkContext Network context to connect.
 * @param[in] pServerAddr Server address to connect to.
 *
 * @return noErr on success, error code on failure.
 */
OSStatus MacSSL_ConnectNetworkContext( NetworkContext_t * pNetworkContext,
                                       InetAddress * pServerAddr )
{
    OSStatus err;

    if( (pNetworkContext == NULL) || (pServerAddr == NULL) )
    {
        return paramErr;
    }

    /* Store server address */
    pNetworkContext->serverAddr = *pServerAddr;

    /* Use our existing SSL_Connect function */
    err = SSL_Connect( pNetworkContext->pSSLState,
                       pServerAddr,
                       pNetworkContext->hostname,
                       NULL,
                       pNetworkContext->logFunc );

    if( err == noErr )
    {
        pNetworkContext->isConnected = true;
        if (pNetworkContext->logFunc) {
            pNetworkContext->logFunc("MacSSL_ConnectNetworkContext: SSL connection established");
        }
    }
    else
    {
        if (pNetworkContext->logFunc) {
            char errorMsg[100];
            sprintf(errorMsg, "MacSSL_ConnectNetworkContext: SSL_Connect failed %d", (int)err);
            pNetworkContext->logFunc(errorMsg);
        }
        pNetworkContext->isConnected = false;
    }

    return err;
}

/**
 * @brief Disconnect and cleanup the network context.
 *
 * @param[in,out] pNetworkContext Network context to disconnect.
 */
void MacSSL_DisconnectNetworkContext( NetworkContext_t * pNetworkContext )
{
    if( pNetworkContext != NULL )
    {
        if( pNetworkContext->isConnected && (pNetworkContext->pSSLState != NULL) )
        {
            SSL_Close( pNetworkContext->pSSLState );
            if (pNetworkContext->logFunc) {
                pNetworkContext->logFunc("MacSSL_DisconnectNetworkContext: SSL connection closed");
            }
        }

        pNetworkContext->isConnected = false;
        pNetworkContext->pSSLState = NULL;
    }
}

/**
 * @brief Setup a TransportInterface_t with our Mac-specific implementations.
 *
 * @param[out] pTransportInterface Transport interface to setup.
 * @param[in] pNetworkContext Network context to use.
 */
void MacSSL_SetupTransportInterface( TransportInterface_t * pTransportInterface,
                                     NetworkContext_t * pNetworkContext )
{
    if( (pTransportInterface != NULL) && (pNetworkContext != NULL) )
    {
        pTransportInterface->recv = MacSSL_TransportRecv;
        pTransportInterface->send = MacSSL_TransportSend;
        pTransportInterface->writev = NULL;  /* Not implemented for now */
        pTransportInterface->pNetworkContext = pNetworkContext;
    }
}