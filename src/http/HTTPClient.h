/*
 * MacSSL - coreHTTP Client Header for Classic Mac OS
 *
 * This header declares the HTTP client functions that use the coreHTTP
 * library for robust and standards-compliant HTTP communication.
 */

#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <Types.h>
#include <OpenTptInternet.h>
#include "../transport/MacHTTPTransport.h"
#include "core_http_client.h"
#include "../common/ProtocolTypes.h"

typedef void (*LoggingCallback)(const char* message);

/* coreHTTP requires additional buffer space beyond the raw response data for:
 * - Internal parsing structures and metadata
 * - HTTP header parsing before body size is known
 * - Protocol compliance (chunked encoding, etc.)
 * - Safety margin for responses slightly larger than expected
 * Industry standard is 10-20% overhead with 1KB minimum for robust HTTP parsing.
 */
#define COREHTTP_PARSING_OVERHEAD_BYTES    1024

typedef struct {
    NetworkContext_t networkContext;      /* Network context for connection */
    TransportInterface_t transport;       /* Transport interface */
    SSLState* pSSLState;                 /* SSL state reference */
    Boolean isConnected;                 /* Connection status */
    char hostname[256];                  /* Current hostname */
    int port;                           /* Current port */
    uint8_t requestHeaderBuffer[1024];   /* Buffer for request headers */
    uint8_t responseBuffer[RESPONSE_BUFFER_SIZE + COREHTTP_PARSING_OVERHEAD_BYTES]; /* Buffer for responses with parsing overhead */
    LoggingCallback logFunc;             /* Logging callback */
    HTTPRequestHeaders_t requestHeaders; /* coreHTTP request headers */
} HTTPClientState;

typedef struct {
    int statusCode;                      /* HTTP status code */
    uint8_t* pBuffer;                   /* Response buffer */
    size_t bufferLen;                   /* Buffer length */
    size_t headersLen;                  /* Headers length */
    size_t bodyLen;                     /* Body length */
    size_t contentLength;               /* Content-Length header value */
} HTTPResponse;

/* HTTP Request Structure */
typedef struct {
    const char* pMethod;                /* HTTP method */
    size_t methodLen;                   /* Method length */
    const char* pPath;                  /* Request path */
    size_t pathLen;                     /* Path length */
    const void* pBody;                  /* Request body */
    size_t bodyLen;                     /* Body length */
} HTTPRequest;

/**
 * @brief Initialize HTTP client state.
 *
 * @param[out] state HTTP client state to initialize.
 * @param[in] sslState SSL state to use for connections.
 * @param[in] logFunc Logging callback function.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpInit(HTTPClientState* state, SSLState* sslState, LoggingCallback logFunc);

/**
 * @brief Connect to a server using HTTPS.
 *
 * @param[in,out] state HTTP client state.
 * @param[in] hostname Server hostname.
 * @param[in] port Server port (usually 443 for HTTPS).
 * @param[in] inetService OpenTransport internet service reference.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpConnect(HTTPClientState* state, const char* hostname, int port, InetSvcRef inetService);

/**
 * @brief Close HTTP connection and cleanup.
 *
 * @param[in,out] state HTTP client state to close.
 */
void HttpClose(HTTPClientState* state);

/**
 * @brief Send HTTP GET request.
 *
 * @param[in] state HTTP client state.
 * @param[in] path Request path.
 * @param[out] response Response structure to fill.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpGet(HTTPClientState* state, const char* path, HTTPResponse* response);

/**
 * @brief Send HTTP POST request.
 *
 * @param[in] state HTTP client state.
 * @param[in] path Request path.
 * @param[in] body Request body data.
 * @param[in] bodyLen Request body length.
 * @param[out] response Response structure to fill.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpPost(HTTPClientState* state, const char* path, const void* body, size_t bodyLen, HTTPResponse* response);

/**
 * @brief Send HTTP PUT request.
 *
 * @param[in] state HTTP client state.
 * @param[in] path Request path.
 * @param[in] body Request body data.
 * @param[in] bodyLen Request body length.
 * @param[out] response Response structure to fill.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpPut(HTTPClientState* state, const char* path, const void* body, size_t bodyLen, HTTPResponse* response);

/**
 * @brief Send HTTP DELETE request.
 *
 * @param[in] state HTTP client state.
 * @param[in] path Request path.
 * @param[out] response Response structure to fill.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpDelete(HTTPClientState* state, const char* path, HTTPResponse* response);

/**
 * @brief Send custom HTTP request.
 *
 * @param[in] state HTTP client state.
 * @param[in] request Request structure.
 * @param[out] response Response structure to fill.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpSendRequest(HTTPClientState* state, const HTTPRequest* request, HTTPResponse* response);

/**
 * @brief Callback function type for processing streaming response data.
 *
 * @param[in] data Response data chunk.
 * @param[in] dataLen Length of the data chunk.
 * @param[in] isComplete Boolean indicating if this is the final chunk.
 * @param[in] userContext User-provided context pointer.
 *
 * @return OSStatus error code (noErr to continue, other values to abort)
 */
typedef OSStatus (*StreamingResponseCallback)(const uint8_t* data, size_t dataLen, Boolean isComplete, void* userContext);

/**
 * @brief Send HTTP GET request with streaming response handling.
 *
 * @param[in] state HTTP client state.
 * @param[in] path Request path.
 * @param[in] callback Callback function to process response chunks.
 * @param[in] userContext User context passed to callback.
 * @param[out] response Response structure to fill with final response info.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpGetStreaming(HTTPClientState* state, const char* path, StreamingResponseCallback callback, void* userContext, HTTPResponse* response);

/**
 * @brief Set a custom HTTP header.
 *
 * @param[in,out] state HTTP client state.
 * @param[in] name Header name.
 * @param[in] value Header value.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpSetHeader(HTTPClientState* state, const char* name, const char* value);

/**
 * @brief Clear all custom HTTP headers.
 *
 * @param[in,out] state HTTP client state.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus HttpClearHeaders(HTTPClientState* state);

/* Demo functions moved to demo/DemoHTTP.h */

#endif /* HTTP_CLIENT_H_ */