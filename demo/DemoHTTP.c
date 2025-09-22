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

    switch (gSelectedHTTPMethod) {
        case kHTTPMethodGET:
            err = HttpGet(&clientState, path, &response);
            sprintf(statusMsg, "coreHTTP: Sending GET request to %s", path);
            break;
        case kHTTPMethodPOST:
            {
                const char* postBody = "{\"message\":\"Hello from PostMac!\"}";
                err = HttpPost(&clientState, path, postBody, strlen(postBody), &response);
                sprintf(statusMsg, "coreHTTP: Sending POST request to %s", path);
            }
            break;
        case kHTTPMethodPUT:
            {
                const char* putBody = "{\"data\":\"Updated from PostMac\"}";
                err = HttpPut(&clientState, path, putBody, strlen(putBody), &response);
                sprintf(statusMsg, "coreHTTP: Sending PUT request to %s", path);
            }
            break;
        case kHTTPMethodDELETE:
            err = HttpDelete(&clientState, path, &response);
            sprintf(statusMsg, "coreHTTP: Sending DELETE request to %s", path);
            break;
        default:
            err = HttpGet(&clientState, path, &response);
            sprintf(statusMsg, "coreHTTP: Sending GET request to %s (default)", path);
            break;
    }

    AppendLogText(statusMsg);

    if (err != noErr) {
        sprintf(statusMsg, "coreHTTP: Failed to send request. Error: %d", (int)err);
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

