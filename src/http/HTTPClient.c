/*
 * MacSSL - coreHTTP Client Implementation for Classic Mac OS
 *
 * This file provides HTTP client functionality using the coreHTTP library
 * for robust and standards-compliant HTTP communication on Classic Mac OS.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* Our HTTP client header (includes coreHTTP) */
#include "HTTPClient.h"

/* Our existing headers */
#include "../ssl/SSLWrapper.h"
/* Removed dependency on demo/Globals.h for library build */
#include "../libyuarel/yuarel.h"
#include "../common/ProtocolTypes.h"

/* Mac OS headers */
#include <Types.h>
#include <OpenTptInternet.h>

/* Forward declare functions defined elsewhere */
extern ProtocolType GetProtocolFromURL(const char* url);
extern int ParseURL(const char* url, char* hostname, char* path, size_t hostnameSize, size_t pathSize);

/* coreHTTP requires additional buffer space beyond the raw response data for:
 * - Internal parsing structures and metadata
 * - HTTP header parsing before body size is known
 * - Protocol compliance (chunked encoding, etc.)
 * - Safety margin for responses slightly larger than expected
 * Industry standard is 10-20% overhead with 1KB minimum for robust HTTP parsing.
 */
#define COREHTTP_PARSING_OVERHEAD_BYTES    1024

/* Global variables for the new HTTP client */
static NetworkContext_t gNewNetworkContext;
static TransportInterface_t gTransportInterface;
static uint8_t gRequestHeaderBuffer[1024];
static uint8_t gNewResponseBuffer[RESPONSE_BUFFER_SIZE + COREHTTP_PARSING_OVERHEAD_BYTES];

/**
 * @brief Convert HTTPStatus_t to a readable string for logging.
 */
static const char* HTTPStatusToString(HTTPStatus_t status)
{
    switch (status)
    {
        case HTTPSuccess:
            return "HTTPSuccess";
        case HTTPInvalidParameter:
            return "HTTPInvalidParameter";
        case HTTPNetworkError:
            return "HTTPNetworkError";
        case HTTPPartialResponse:
            return "HTTPPartialResponse";
        case HTTPNoResponse:
            return "HTTPNoResponse";
        case HTTPInsufficientMemory:
            return "HTTPInsufficientMemory";
        case HTTPSecurityAlertExtraneousResponseData:
            return "HTTPSecurityAlertExtraneousResponseData";
        case HTTPSecurityAlertInvalidChunkHeader:
            return "HTTPSecurityAlertInvalidChunkHeader";
        case HTTPSecurityAlertInvalidProtocolVersion:
            return "HTTPSecurityAlertInvalidProtocolVersion";
        case HTTPSecurityAlertInvalidStatusCode:
            return "HTTPSecurityAlertInvalidStatusCode";
        case HTTPSecurityAlertInvalidCharacter:
            return "HTTPSecurityAlertInvalidCharacter";
        case HTTPSecurityAlertInvalidContentLength:
            return "HTTPSecurityAlertInvalidContentLength";
        case HTTPParserPaused:
            return "HTTPParserPaused";
        case HTTPParserInternalError:
            return "HTTPParserInternalError";
        case HTTPHeaderNotFound:
            return "HTTPHeaderNotFound";
        default:
            return "Unknown HTTP Status";
    }
}

/**
 * @brief ConnectToServer implementation using coreHTTP.
 *
 * This function connects to a server and fetches data using the coreHTTP
 * library for robust HTTP handling. Now uses the new granular interface internally.
 *
 * @return OSStatus error code (noErr on success)
 */
/* ConnectToServer function moved to demo/DemoHTTP.c */

/**
 * @brief TestSSLHandshake implementation using coreHTTP transport layer.
 *
 * This function tests just the SSL connection without sending HTTP data,
 * using the coreHTTP transport layer for connection management.
 *
 * @return OSStatus error code (noErr on success)
 */
/* TestSSLHandshake function moved to demo/DemoHTTP.c */

/**
 * @brief Initialize HTTP client state.
 */
OSStatus HttpInit(HTTPClientState* state, SSLState* sslState, LoggingCallback logFunc)
{
    if (state == NULL) {
        return paramErr;
    }

    /* Clear the state structure */
    memset(state, 0, sizeof(HTTPClientState));

    /* Store the logging callback */
    state->logFunc = logFunc;

    /* Initialize the SSL state pointer */
    state->pSSLState = sslState;

    /* Set up request headers structure */
    state->requestHeaders.pBuffer = state->requestHeaderBuffer;
    state->requestHeaders.bufferLen = sizeof(state->requestHeaderBuffer);

    /* Mark as not connected */
    state->isConnected = false;

    if (logFunc) {
        logFunc("HttpInit: HTTP client state initialized");
    }

    return noErr;
}

/**
 * @brief Connect to a server using HTTPS.
 */
OSStatus HttpConnect(HTTPClientState* state, const char* hostname, int port, InetSvcRef inetService)
{
    OSStatus err = noErr;
    InetHostInfo hostInfo;
    InetAddress inAddr;
    char statusMsg[256];

    if (state == NULL || hostname == NULL) {
        return paramErr;
    }

    if (state->logFunc) {
        sprintf(statusMsg, "HttpConnect: Connecting to %s:%d", hostname, port);
        state->logFunc(statusMsg);
    }

    /* Store connection parameters */
    strncpy(state->hostname, hostname, sizeof(state->hostname) - 1);
    state->hostname[sizeof(state->hostname) - 1] = '\0';
    state->port = port;

    /* Initialize SSL if not already done */
    err = SSL_Initialize(state->pSSLState, state->logFunc);
    if (err != noErr) {
        if (state->logFunc) {
            sprintf(statusMsg, "HttpConnect: SSL initialization failed. Error: %d", (int)err);
            state->logFunc(statusMsg);
        }
        return err;
    }

    /* Look up the host address (cast away const for OT API) */
    err = OTInetStringToAddress(inetService, (char*)hostname, &hostInfo);
    if (err != noErr) {
        if (state->logFunc) {
            state->logFunc("HttpConnect: Could not resolve host address");
        }
        return err;
    }

    /* Set up the address for the remote host */
    OTInitInetAddress(&inAddr, port, hostInfo.addrs[0]);

    /* Initialize network context */
    err = MacSSL_InitializeNetworkContext(&state->networkContext, state->pSSLState, hostname, state->logFunc);
    if (err != noErr) {
        if (state->logFunc) {
            sprintf(statusMsg, "HttpConnect: Failed to initialize network context. Error: %d", (int)err);
            state->logFunc(statusMsg);
        }
        return err;
    }

    /* Connect using our transport layer */
    err = MacSSL_ConnectNetworkContext(&state->networkContext, &inAddr);
    if (err != noErr) {
        if (state->logFunc) {
            sprintf(statusMsg, "HttpConnect: Failed to connect. Error: %d", (int)err);
            state->logFunc(statusMsg);
        }
        return err;
    }

    /* Setup transport interface */
    MacSSL_SetupTransportInterface(&state->transport, &state->networkContext);

    /* Mark as connected */
    state->isConnected = true;

    if (state->logFunc) {
        state->logFunc("HttpConnect: Connected successfully");
    }

    return noErr;
}

/**
 * @brief Close HTTP connection and cleanup.
 */
void HttpClose(HTTPClientState* state)
{
    if (state == NULL) {
        return;
    }

    if (state->isConnected) {
        /* Disconnect the network context */
        MacSSL_DisconnectNetworkContext(&state->networkContext);
        state->isConnected = false;

        if (state->logFunc) {
            state->logFunc("HttpClose: Connection closed");
        }
    }

    /* Clear connection parameters */
    memset(state->hostname, 0, sizeof(state->hostname));
    state->port = 0;
}

/**
 * @brief Internal helper to send HTTP request using coreHTTP.
 */
static OSStatus HttpSendRequestInternal(HTTPClientState* state, const HTTPRequest* request, HTTPResponse* response)
{
    HTTPStatus_t httpStatus;
    HTTPRequestInfo_t requestInfo;
    HTTPResponse_t coreResponse;
    char statusMsg[256];

    if (state == NULL || request == NULL || response == NULL) {
        return paramErr;
    }

    if (!state->isConnected) {
        if (state->logFunc) {
            state->logFunc("HttpSendRequestInternal: Not connected to server");
        }
        return notOpenErr;
    }

    /* Initialize request info structure */
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.pMethod = request->pMethod;
    requestInfo.methodLen = request->methodLen;
    requestInfo.pPath = request->pPath;
    requestInfo.pathLen = request->pathLen;
    requestInfo.pHost = state->hostname;
    requestInfo.hostLen = strlen(state->hostname);
    requestInfo.reqFlags = 0;

    /* Initialize request headers */
    httpStatus = HTTPClient_InitializeRequestHeaders(&state->requestHeaders, &requestInfo);
    if (httpStatus != HTTPSuccess) {
        if (state->logFunc) {
            sprintf(statusMsg, "HttpSendRequestInternal: Failed to initialize request headers: %s",
                    HTTPStatusToString(httpStatus));
            state->logFunc(statusMsg);
        }
        return -1;
    }

    /* Initialize response structure */
    memset(&coreResponse, 0, sizeof(coreResponse));
    coreResponse.pBuffer = state->responseBuffer;
    coreResponse.bufferLen = sizeof(state->responseBuffer);

    /* Send HTTP request and receive response */
    if (state->logFunc) {
        sprintf(statusMsg, "HttpSendRequestInternal: Sending %.*s request to %.*s",
                (int)request->methodLen, request->pMethod,
                (int)request->pathLen, request->pPath);
        state->logFunc(statusMsg);
    }

    httpStatus = HTTPClient_Send(&state->transport,
                                 &state->requestHeaders,
                                 (const uint8_t*)request->pBody,
                                 request->bodyLen,
                                 &coreResponse,
                                 0);

    if (httpStatus == HTTPSuccess) {
        /* Fill response structure */
        response->statusCode = coreResponse.statusCode;
        response->pBuffer = coreResponse.pBuffer;
        response->bufferLen = coreResponse.bufferLen;
        response->headersLen = coreResponse.headersLen;
        response->bodyLen = coreResponse.bodyLen;
        response->contentLength = coreResponse.contentLength;

        if (state->logFunc) {
            sprintf(statusMsg, "HttpSendRequestInternal: Request successful! Status: %d", response->statusCode);
            state->logFunc(statusMsg);
        }

        return noErr;
    } else {
        if (state->logFunc) {
            sprintf(statusMsg, "HttpSendRequestInternal: Request failed: %s", HTTPStatusToString(httpStatus));
            state->logFunc(statusMsg);
        }
        return -1;
    }
}

/**
 * @brief Send HTTP GET request.
 */
OSStatus HttpGet(HTTPClientState* state, const char* path, HTTPResponse* response)
{
    HTTPRequest request;

    if (state == NULL || path == NULL || response == NULL) {
        return paramErr;
    }

    /* Set up GET request */
    request.pMethod = HTTP_METHOD_GET;
    request.methodLen = strlen(HTTP_METHOD_GET);
    request.pPath = path;
    request.pathLen = strlen(path);
    request.pBody = NULL;
    request.bodyLen = 0;

    return HttpSendRequestInternal(state, &request, response);
}

/**
 * @brief Send HTTP POST request.
 */
OSStatus HttpPost(HTTPClientState* state, const char* path, const void* body, size_t bodyLen, HTTPResponse* response)
{
    HTTPRequest request;

    if (state == NULL || path == NULL || response == NULL) {
        return paramErr;
    }

    /* Set up POST request */
    request.pMethod = HTTP_METHOD_POST;
    request.methodLen = strlen(HTTP_METHOD_POST);
    request.pPath = path;
    request.pathLen = strlen(path);
    request.pBody = body;
    request.bodyLen = bodyLen;

    return HttpSendRequestInternal(state, &request, response);
}

/**
 * @brief Send HTTP PUT request.
 */
OSStatus HttpPut(HTTPClientState* state, const char* path, const void* body, size_t bodyLen, HTTPResponse* response)
{
    HTTPRequest request;

    if (state == NULL || path == NULL || response == NULL) {
        return paramErr;
    }

    /* Set up PUT request */
    request.pMethod = HTTP_METHOD_PUT;
    request.methodLen = strlen(HTTP_METHOD_PUT);
    request.pPath = path;
    request.pathLen = strlen(path);
    request.pBody = body;
    request.bodyLen = bodyLen;

    return HttpSendRequestInternal(state, &request, response);
}

/**
 * @brief Send HTTP DELETE request.
 */
OSStatus HttpDelete(HTTPClientState* state, const char* path, HTTPResponse* response)
{
    HTTPRequest request;

    if (state == NULL || path == NULL || response == NULL) {
        return paramErr;
    }

    /* Set up DELETE request */
    request.pMethod = "DELETE";
    request.methodLen = strlen("DELETE");
    request.pPath = path;
    request.pathLen = strlen(path);
    request.pBody = NULL;
    request.bodyLen = 0;

    return HttpSendRequestInternal(state, &request, response);
}

/**
 * @brief Send custom HTTP request.
 */
OSStatus HttpSendRequest(HTTPClientState* state, const HTTPRequest* request, HTTPResponse* response)
{
    if (state == NULL || request == NULL || response == NULL) {
        return paramErr;
    }

    return HttpSendRequestInternal(state, request, response);
}

/**
 * @brief Set a custom HTTP header.
 */
OSStatus HttpSetHeader(HTTPClientState* state, const char* name, const char* value)
{
    HTTPStatus_t httpStatus;
    char statusMsg[256];

    if (state == NULL || name == NULL || value == NULL) {
        return paramErr;
    }

    /* Add header to the request headers */
    httpStatus = HTTPClient_AddHeader(&state->requestHeaders, name, strlen(name), value, strlen(value));
    if (httpStatus != HTTPSuccess) {
        if (state->logFunc) {
            sprintf(statusMsg, "HttpSetHeader: Failed to add header %s: %s", name, HTTPStatusToString(httpStatus));
            state->logFunc(statusMsg);
        }
        return -1;
    }

    if (state->logFunc) {
        sprintf(statusMsg, "HttpSetHeader: Added header %s: %s", name, value);
        state->logFunc(statusMsg);
    }

    return noErr;
}

/**
 * @brief Clear all custom HTTP headers.
 */
OSStatus HttpClearHeaders(HTTPClientState* state)
{
    if (state == NULL) {
        return paramErr;
    }

    /* Reset the headers length - this effectively clears headers */
    state->requestHeaders.headersLen = 0;

    if (state->logFunc) {
        state->logFunc("HttpClearHeaders: Cleared all custom headers");
    }

    return noErr;
}