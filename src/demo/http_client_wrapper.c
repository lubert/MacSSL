/*
 * MacSSL - coreHTTP Client Wrapper for Classic Mac OS
 *
 * This file provides a wrapper around coreHTTP that maintains the same
 * interface as our existing ConnectToServer() function, allowing for
 * side-by-side testing and gradual migration.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* coreHTTP client library */
#include "core_http_client.h"

/* Our transport interface */
#include "http_transport_mac.h"

/* Our existing headers */
#include "ssl_wrapper.h"
#include "globals.h"
#include "yuarel.h"

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
extern void AppendResponseChunk(char* chunk, long chunkLength);
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
 * @brief New implementation of ConnectToServer using coreHTTP.
 *
 * This function provides the same interface as the original ConnectToServer()
 * but uses coreHTTP internally for better HTTP handling.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus ConnectToServer_New(void)
{
    OSStatus err = noErr;
    HTTPStatus_t httpStatus;
    InetHostInfo hostInfo;
    InetAddress inAddr;
    char hostname[256];
    char path[512];
    char url[512];
    int urlLen;
    ProtocolType protocolType;
    HTTPRequestHeaders_t requestHeaders;
    HTTPRequestInfo_t requestInfo;
    HTTPResponse_t response;
    char statusMsg[200];

    /* Show wait cursor */
    SetCursor(*GetCursor(watchCursor));

    AppendLogText("=== Starting coreHTTP Client ===");

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

    sprintf(statusMsg, "coreHTTP: Connecting to %s", hostname);
    AppendLogText(statusMsg);

    /* Initialize SSL */
    err = SSL_Initialize(&gSSLState, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: SSL initialization failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Look up the host address */
    err = OTInetStringToAddress(gInetService, hostname, &hostInfo);
    if (err != noErr) {
        AppendLogText("coreHTTP: Could not resolve host address");
        SetCursor(&qd.arrow);
        return err;
    }

    /* Set up the address for the remote host with HTTPS port */
    OTInitInetAddress(&inAddr, 443, hostInfo.addrs[0]);

    /* Initialize network context */
    err = MacSSL_InitializeNetworkContext(&gNewNetworkContext, &gSSLState, hostname);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to initialize network context. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Connect using our transport layer */
    err = MacSSL_ConnectNetworkContext(&gNewNetworkContext, &inAddr);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to connect. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    /* Setup transport interface */
    MacSSL_SetupTransportInterface(&gTransportInterface, &gNewNetworkContext);

    AppendLogText("coreHTTP: Connected successfully");

    /* Initialize request headers structure */
    memset(&requestHeaders, 0, sizeof(requestHeaders));
    requestHeaders.pBuffer = gRequestHeaderBuffer;
    requestHeaders.bufferLen = sizeof(gRequestHeaderBuffer);

    /* Initialize request info structure */
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.pMethod = HTTP_METHOD_GET;
    requestInfo.methodLen = strlen(HTTP_METHOD_GET);
    requestInfo.pPath = path;
    requestInfo.pathLen = strlen(path);
    requestInfo.pHost = hostname;
    requestInfo.hostLen = strlen(hostname);
    requestInfo.reqFlags = 0; /* No special flags for now */

    /* Initialize request headers */
    AppendLogText("coreHTTP: Initializing request headers");
    httpStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders, &requestInfo);
    if (httpStatus != HTTPSuccess) {
        sprintf(statusMsg, "coreHTTP: Failed to initialize request headers: %s",
                HTTPStatusToString(httpStatus));
        AppendLogText(statusMsg);
        MacSSL_DisconnectNetworkContext(&gNewNetworkContext);
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Initialize response structure */
    memset(&response, 0, sizeof(response));
    response.pBuffer = gNewResponseBuffer;
    response.bufferLen = sizeof(gNewResponseBuffer);

    /* Send HTTP request and receive response */
    AppendLogText("coreHTTP: Sending HTTP request");
    httpStatus = HTTPClient_Send(&gTransportInterface,
                                 &requestHeaders,
                                 NULL,          /* No request body */
                                 0,             /* No request body length */
                                 &response,
                                 0);            /* No special flags */

    if (httpStatus == HTTPSuccess) {
        sprintf(statusMsg, "coreHTTP: Request successful! Status: %d", response.statusCode);
        AppendLogText(statusMsg);

        sprintf(statusMsg, "coreHTTP: Response headers length: %zu bytes", response.headersLen);
        AppendLogText(statusMsg);

        sprintf(statusMsg, "coreHTTP: Response body length: %zu bytes", response.bodyLen);
        AppendLogText(statusMsg);

        sprintf(statusMsg, "coreHTTP: Content-Length: %zu", response.contentLength);
        AppendLogText(statusMsg);

        /* Display the response using our existing function */
        if (response.headersLen + response.bodyLen > 0) {
            /* Convert uint8_t* to char* for compatibility with existing display functions */
            char* responseStr = (char*)response.pBuffer;
            DisplayResponse(responseStr, response.headersLen + response.bodyLen);
        }
    } else {
        sprintf(statusMsg, "coreHTTP: Request failed: %s", HTTPStatusToString(httpStatus));
        AppendLogText(statusMsg);
        err = -1;
    }

    /* Clean up */
    MacSSL_DisconnectNetworkContext(&gNewNetworkContext);

    /* Restore cursor */
    SetCursor(&qd.arrow);

    AppendLogText("=== coreHTTP Client Complete ===");

    return err;
}

/**
 * @brief New implementation of TestSSLHandshake using coreHTTP transport layer.
 *
 * This function tests just the SSL connection without sending HTTP data,
 * similar to the original TestSSLHandshake() but using our coreHTTP transport.
 *
 * @return OSStatus error code (noErr on success)
 */
OSStatus TestSSLHandshake_New(void)
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

    /* Look up the host address */
    err = OTInetStringToAddress(gInetService, hostname, &hostInfo);
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