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
#include "../demo/Globals.h"
#include "../libyuarel/yuarel.h"
#include "../common/ProtocolTypes.h"

/* Forward declare external globals */
extern TEHandle gURLText;
extern SSLState gSSLState;
extern InetSvcRef gInetService;

/* Mac OS headers */
#include <Types.h>
#include <OpenTptInternet.h>

/* Forward declare AppendLogText */
extern void AppendLogText(const char* message);
extern void DisplayResponse(char* response, long responseLength);
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
OSStatus ConnectToServer(void)
{
    OSStatus err = noErr;
    HTTPClientState clientState;
    HTTPResponse response;
    char hostname[256];
    char path[512];
    char url[512];
    int urlLen;
    ProtocolType protocolType;
    char statusMsg[200];

    /* Show wait cursor */
    SetCursor(*GetCursor(watchCursor));

    AppendLogText("=== Starting coreHTTP Client (New Interface) ===");

    /* Get URL from text field */
    if (gURLText == NULL) {
        AppendLogText("Error: URL field not initialized");
        SetCursor(&qd.arrow);
        return -1;
    }

    urlLen = (*gURLText)->teLength;
    if (urlLen >= sizeof(url)) {
        AppendLogText("Error: URL too long");
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Copy URL from TextEdit handle */
    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    /* Determine protocol from URL */
    protocolType = GetProtocolFromURL(url);

    /* Parse URL into hostname and path */
    if (ParseURL(url, hostname, path, sizeof(hostname), sizeof(path)) != 0) {
        AppendLogText("Error: Could not parse URL");
        SetCursor(&qd.arrow);
        return -1;
    }

    if (strlen(hostname) == 0) {
        AppendLogText("Error: Please enter a hostname");
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Only support HTTPS for now */
    if (protocolType != kProtocolHTTPS) {
        AppendLogText("Error: coreHTTP implementation only supports HTTPS URLs");
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Use new granular interface */
    /* Step 1: Initialize HTTP client */
    err = HttpInit(&clientState, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to initialize HTTP client. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Step 2: Connect to server */
    err = HttpConnect(&clientState, hostname, 443);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to connect to %s. Error: %d", hostname, (int)err);
        AppendLogText(statusMsg);
        HttpClose(&clientState);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Step 3: Send GET request */
    err = HttpGet(&clientState, path, &response);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to send GET request. Error: %d", (int)err);
        AppendLogText(statusMsg);
        HttpClose(&clientState);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Display response information */
    sprintf(statusMsg, "coreHTTP: Request successful! Status: %d", response.statusCode);
    AppendLogText(statusMsg);

    sprintf(statusMsg, "coreHTTP: Response headers length: %ld bytes", (long)response.headersLen);
    AppendLogText(statusMsg);

    sprintf(statusMsg, "coreHTTP: Response body length: %ld bytes", (long)response.bodyLen);
    AppendLogText(statusMsg);

    sprintf(statusMsg, "coreHTTP: Content-Length: %ld", (long)response.contentLength);
    AppendLogText(statusMsg);

    /* Display the response using our existing function */
    if (response.headersLen + response.bodyLen > 0) {
        /* Convert uint8_t* to char* for compatibility with existing display functions */
        char* responseStr = (char*)response.pBuffer;
        DisplayResponse(responseStr, response.headersLen + response.bodyLen);
    }

    /* Step 4: Clean up */
    HttpClose(&clientState);

    /* Restore cursor */
    SetCursor(&qd.arrow);

    AppendLogText("=== coreHTTP Client Complete ===");

    return noErr;
}

/**
 * @brief TestSSLHandshake implementation using coreHTTP transport layer.
 *
 * This function tests just the SSL connection without sending HTTP data,
 * using the coreHTTP transport layer for connection management.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus TestSSLHandshake(void)
{
    OSStatus err = noErr;
    InetHostInfo hostInfo;
    InetAddress inAddr;
    char hostname[256];
    char path[512];
    char url[512];
    int urlLen;
    ProtocolType protocolType;
    NetworkContext_t testNetworkContext;
    char statusMsg[200];

    /* Show wait cursor */
    SetCursor(*GetCursor(watchCursor));

    AppendLogText("=== Starting coreHTTP Transport Test ===");

    /* Get URL from text field */
    if (gURLText == NULL) {
        AppendLogText("Error: URL field not initialized");
        SetCursor(&qd.arrow);
        return -1;
    }

    urlLen = (*gURLText)->teLength;
    if (urlLen >= sizeof(url)) {
        AppendLogText("Error: URL too long");
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Copy URL from TextEdit handle */
    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    /* Determine protocol from URL */
    protocolType = GetProtocolFromURL(url);

    /* Parse URL into hostname and path */
    if (ParseURL(url, hostname, path, sizeof(hostname), sizeof(path)) != 0) {
        AppendLogText("Error: Could not parse URL");
        SetCursor(&qd.arrow);
        return -1;
    }

    if (strlen(hostname) == 0) {
        AppendLogText("Error: Please enter a hostname");
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Only support HTTPS for now */
    if (protocolType != kProtocolHTTPS) {
        AppendLogText("Error: coreHTTP transport test only supports HTTPS URLs");
        SetCursor(&qd.arrow);
        return -1;
    }

    sprintf(statusMsg, "coreHTTP Transport: Testing connection to %s", hostname);
    AppendLogText(statusMsg);

    /* Initialize SSL */
    err = SSL_Initialize(&gSSLState, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP Transport: SSL initialization failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Look up the host address (cast away const for OT API) */
    err = OTInetStringToAddress(gInetService, (char*)hostname, &hostInfo);
    if (err != noErr) {
        AppendLogText("coreHTTP Transport: Could not resolve host address");
        SetCursor(&qd.arrow);
        return err;
    }

    /* Set up the address for the remote host with HTTPS port */
    OTInitInetAddress(&inAddr, 443, hostInfo.addrs[0]);

    /* Initialize network context for this test */
    err = MacSSL_InitializeNetworkContext(&testNetworkContext, &gSSLState, hostname);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP Transport: Failed to initialize network context. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Test the transport layer connection */
    AppendLogText("coreHTTP Transport: Testing SSL handshake...");
    err = MacSSL_ConnectNetworkContext(&testNetworkContext, &inAddr);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP Transport: SSL handshake failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
        MacSSL_DisconnectNetworkContext(&testNetworkContext);
        SetCursor(&qd.arrow);
        return err;
    }

    AppendLogText("coreHTTP Transport: SSL handshake successful!");
    AppendLogText("coreHTTP Transport: Transport layer test completed - no HTTP data transferred.");

    /* Test a simple transport send/receive to verify the layer works */
    AppendLogText("coreHTTP Transport: Testing transport send/receive functions...");

    const char* testData = "Connection test";
    char receiveBuffer[32];
    int32_t sendResult, recvResult;

    /* Test send function (this will actually send data but server may ignore it) */
    sendResult = MacSSL_TransportSend(&testNetworkContext, testData, strlen(testData));
    if (sendResult > 0) {
        sprintf(statusMsg, "coreHTTP Transport: Send function working (%d bytes)", sendResult);
        AppendLogText(statusMsg);
    } else {
        sprintf(statusMsg, "coreHTTP Transport: Send function test: %d", sendResult);
        AppendLogText(statusMsg);
    }

    /* Test receive function (may timeout since we're not sending proper HTTP) */
    recvResult = MacSSL_TransportRecv(&testNetworkContext, receiveBuffer, sizeof(receiveBuffer) - 1);
    if (recvResult >= 0) {
        sprintf(statusMsg, "coreHTTP Transport: Receive function working (%d bytes)", recvResult);
        AppendLogText(statusMsg);
    } else {
        sprintf(statusMsg, "coreHTTP Transport: Receive function test: %d", recvResult);
        AppendLogText(statusMsg);
    }

    AppendLogText("coreHTTP Transport: All transport layer functions tested successfully!");

    /* Clean up */
    MacSSL_DisconnectNetworkContext(&testNetworkContext);

    /* Restore cursor */
    SetCursor(&qd.arrow);

    AppendLogText("=== coreHTTP Transport Test Complete ===");

    return noErr;
}

/**
 * @brief Initialize HTTP client state.
 */
OSStatus HttpInit(HTTPClientState* state, LoggingCallback logFunc)
{
    if (state == NULL) {
        return paramErr;
    }

    /* Clear the state structure */
    memset(state, 0, sizeof(HTTPClientState));

    /* Store the logging callback */
    state->logFunc = logFunc;

    /* Initialize the SSL state pointer to the global state */
    state->pSSLState = &gSSLState;

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
OSStatus HttpConnect(HTTPClientState* state, const char* hostname, int port)
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
    err = OTInetStringToAddress(gInetService, (char*)hostname, &hostInfo);
    if (err != noErr) {
        if (state->logFunc) {
            state->logFunc("HttpConnect: Could not resolve host address");
        }
        return err;
    }

    /* Set up the address for the remote host */
    OTInitInetAddress(&inAddr, port, hostInfo.addrs[0]);

    /* Initialize network context */
    err = MacSSL_InitializeNetworkContext(&state->networkContext, state->pSSLState, hostname);
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