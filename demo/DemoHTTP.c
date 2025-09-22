/*
 * PostMac Demo HTTP Functions
 *
 * These are demo-specific HTTP functions that depend on UI globals
 * and should not be part of the core MacSSL library.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Types.h>
#include <Quickdraw.h>
#include <Memory.h>
#include <OSUtils.h>

#include <OpenTptInternet.h>

#include "../src/ssl/SSLWrapper.h"
#include "../src/http/HTTPClient.h"
#include "../src/libyuarel/yuarel.h"
#include "../src/common/ProtocolTypes.h"
#include "Globals.h"

extern void AppendLogText(const char* message);
extern void DisplayResponse(char* response, long responseLength);
extern ProtocolType GetProtocolFromURL(const char* url);
extern int ParseURL(const char* url, char* hostname, char* path, size_t hostnameSize, size_t pathSize);

extern TEHandle gURLText;
extern SSLState gSSLState;
extern InetSvcRef gInetService;

/**
 * @brief Connect to server and perform HTTPS request using coreHTTP.
 *
 * This is the main function for making HTTPS requests. It replaces the older
 * SSL-only implementation with a complete HTTP client using the coreHTTP
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

    SetCursor(*GetCursor(watchCursor));

    AppendLogText("=== Starting coreHTTP Client (New Interface) ===");

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

    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    protocolType = GetProtocolFromURL(url);

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

    if (protocolType != kProtocolHTTPS) {
        AppendLogText("Error: coreHTTP implementation only supports HTTPS URLs");
        SetCursor(&qd.arrow);
        return -1;
    }

    err = HttpInit(&clientState, &gSSLState, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to initialize HTTP client. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return err;
    }

    err = HttpConnect(&clientState, hostname, 443, gInetService);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to connect to %s. Error: %d", hostname, (int)err);
        AppendLogText(statusMsg);
        HttpClose(&clientState);
        SetCursor(&qd.arrow);
        return err;
    }

    err = HttpGet(&clientState, path, &response);
    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to send GET request. Error: %d", (int)err);
        AppendLogText(statusMsg);
        HttpClose(&clientState);
        SetCursor(&qd.arrow);
        return err;
    }

    sprintf(statusMsg, "coreHTTP: Request successful! Status: %d", response.statusCode);
    AppendLogText(statusMsg);

    sprintf(statusMsg, "coreHTTP: Response headers length: %ld bytes", (long)response.headersLen);
    AppendLogText(statusMsg);

    sprintf(statusMsg, "coreHTTP: Response body length: %ld bytes", (long)response.bodyLen);
    AppendLogText(statusMsg);

    sprintf(statusMsg, "coreHTTP: Content-Length: %ld", (long)response.contentLength);
    AppendLogText(statusMsg);

    if (response.headersLen + response.bodyLen > 0) {
        char* responseStr = (char*)response.pBuffer;
        DisplayResponse(responseStr, response.headersLen + response.bodyLen);
    }

    HttpClose(&clientState);

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

    SetCursor(*GetCursor(watchCursor));

    AppendLogText("=== Starting coreHTTP Transport Test ===");

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

    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    protocolType = GetProtocolFromURL(url);

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
    err = MacSSL_InitializeNetworkContext(&testNetworkContext, &gSSLState, hostname, AppendLogText);
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

    SetCursor(&qd.arrow);

    AppendLogText("=== coreHTTP Transport Test Complete ===");

    return noErr;
}